#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryCPUWrapper.hpp"

namespace spike_model {
	const char MemoryCPUWrapper::name[] = "memory_cpu";

	////////////////////////////////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////////////////////////////////

	MemoryCPUWrapper::MemoryCPUWrapper(sparta::TreeNode *node, const MemoryCPUWrapperParameterSet *p):
			sparta::Unit(node),
			line_size_(p->line_size),
			latency_(p->latency),
			sched_mem_req(&controller_cycle_event_mem_req, p->latency),
			sched_outgoing(&controller_cycle_event_outgoing_transaction, p->latency),
			sched_incoming(&controller_cycle_event_incoming_transaction, p->latency) {
				in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_noc_, std::shared_ptr<NoCMessage>));
				in_port_mc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_mc_, std::shared_ptr<CacheRequest>));
				instructionID_counter = 1;
				setID(0);
				
				sp_status = (SPStatus *)calloc(num_of_registers, sizeof(SPStatus));
				sparta_assert(sp_status != nullptr, "SP status array could not be allocated!");
								
				//-- read the configuration
				root_node			= node->getRoot()->getAs<sparta::RootTreeNode>();
				auto config			= root_node->getSimulator()->getSimulationConfiguration()->getUnboundParameterTree();
				this->enabled		= config.tryGet("meta.params.enable_smart_mcpu")->getAs<bool>();
				std::cout << "Memory Tile is " << (this->enabled ? "enabled" : "disabled") << "." << std::endl;
				
				this->sp_reg_size	= (uint64_t)config.tryGet("top.cpu.tile0.params.num_l2_banks")->getAs<uint16_t>() *
									  config.tryGet("top.cpu.tile0.l2_bank0.params.size_kb")->getAs<uint64_t>() /
									  (uint64_t)num_of_registers * (uint64_t)1024;
				if(enabled) {
					std::cout << "Each register entry in the SP is " << sp_reg_size << " Bytes." << std::endl;
				}
				
				//-- TODO This nullptr should not be here. The problem is, that if the constructor of MemoryCPUWrapper is called, 
				//        NoC is not yet initialized and hence the NoC leaf in the tree cannot be accessed. Ideally the 
				//        sequence of initialization is changed.
				//		  The root_node does not need to be a field in the class any longer either, once #98 is fixed.
				//		  @see issue #98
				//		  @see MemoryCPUWrapper::controllerCycle_outgoing_transaction
				//this->noc = root_node->getChild(std::string("cpu.noc"))->getResourceAs<spike_model::NoC>();	
				this->noc      = nullptr;
	}


	/////////////////////////////////////
	//-- Message handling from the NoC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes) {
		std::cout << getClock()->currentCycle() << ": " << name << ": receiveMessage_noc_: Received from NoC: " << *mes << std::endl;
		if(enabled) {
			count_requests_noc_++;
		}
		
		mes->getRequest()->handle(this);
	}


	//-- Bypass for a scalar memory operation
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::CacheRequest> mes) {
		
		std::cout << SPARTA_UNMANAGED_COLOR_CYAN << getClock()->currentCycle() << ": " << name << ": handle CacheRequest: " << *mes << SPARTA_UNMANAGED_COLOR_NORMAL << std::endl;
		if(enabled) {
			//-- Schedule memory request for the MC
			sched_mem_req.push(mes);
		} else {
			//-- whatever comes into the Memory Tile, just send it out to the MC
			out_port_mc_.send(mes, 0);
		}
	}



	//-- A memory transaction to be handled by the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUSetVVL> mes) {
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle an MCPUSetVVL instruction!");
		
		uint64_t elements_per_sp = sp_reg_size / (uint64_t)mes->getWidth() * (uint64_t)mes->getLMUL();
		vvl_ = std::min(elements_per_sp, mes->getAVL());
		//vvl_ = std::min((uint64_t)8, mes->getAVL());
		
		mes->setVVL(vvl_);
			
		mes->setServiced();
		
		std::cout << SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN << getClock()->currentCycle() << ": " << name << ": handle MCPUSetVVL: " << *mes << SPARTA_UNMANAGED_COLOR_NORMAL << std::endl;
			
		std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, line_size_, getID(), mes->getSourceTile());
		sched_outgoing.push(outgoing_noc_message);
	}



	//-- A vector instruction for the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUInstruction> r){
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle an MCPUInstruction!");
		
		std::cout << SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN << getClock()->currentCycle() << ": " << name << ": handle: Memory instruction received: " << *r << SPARTA_UNMANAGED_COLOR_NORMAL << std::endl;
		
		r->setID(this->instructionID_counter);
		
		struct transaction instruction_attributes = {r, 0, 0, 1};
		transaction_table.insert({this->instructionID_counter, instruction_attributes}); // keep track of the transaction
		this->instructionID_counter++; // increment ID
		if(this->instructionID_counter == 0) {this->instructionID_counter = 1;}	// handle the overflow. 0 is reserved for cache requests that use the bypass
		
		//-- schedule the incoming message
		sched_incoming.push(r);
	}


	//-- Handle for Scratchpad requests
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::ScratchpadRequest> r){
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle a ScratchpadRequest!");
		
		std::cout << SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN << getClock()->currentCycle() << ": " << name << ": handle: ScratchpadRequest: " << *r << SPARTA_UNMANAGED_COLOR_NORMAL << std::endl;
		
		sched_incoming.push(r);
	}





	void MemoryCPUWrapper::controllerCycle_incoming_transaction() {
		
		//-- If the incoming transaction is a MCPU instruction, that means, that the VAS Tile instructs the MCPU to perform an operation.
		if(std::shared_ptr<MCPUInstruction> instr_to_schedule = std::dynamic_pointer_cast<MCPUInstruction>(sched_incoming.front())) {
			std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_incoming_transaction: " << *instr_to_schedule;
			
			//-- If it is a load
			if(instr_to_schedule->get_operation() == MCPUInstruction::Operation::LOAD) {
				
				//-- If we have not done before, an ALLOCATE has to be sent to the VAS Tile, so it reserves space for the data to be written to the SP
				if(sp_status[instr_to_schedule->getDestinationRegId()] == SPStatus::IS_L2) {
					sp_status[instr_to_schedule->getDestinationRegId()] = SPStatus::ALLOC_SENT;
					
					std::shared_ptr<ScratchpadRequest> sp_request = createScratchpadRequest(instr_to_schedule, ScratchpadRequest::ScratchpadCommand::ALLOCATE);
					sp_request->setSize(vvl_ * (uint)instr_to_schedule->get_width());	// reserve the space in the VAS Tile
					std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(sp_request, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, getID(), instr_to_schedule->getSourceTile());
					sched_outgoing.push(noc_message);
					
					std::cout << ", sending SP ALLOC: " << *sp_request;
				}
				std::cout << std::endl;
				
				//-- Send the request to the Vector Address Generators (VAG)
				computeMemReqAddresses(instr_to_schedule);
				
			} else { // If it is a store, generate a ScratchpadRequest to load the data from the VAS Tile
				
				std::shared_ptr outgoing_message = createScratchpadRequest(instr_to_schedule, ScratchpadRequest::ScratchpadCommand::READ);
				outgoing_message->setSize(line_size_);
				outgoing_message->setOperandReady(true);
				
				
				std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(outgoing_message, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, getID(), instr_to_schedule->getSourceTile());
				
				//-- TODO issue #94 needs to be fixed first
				//sched_outgoing.push(noc_message);
				
				std::cout << ", sending SP READ: " << *outgoing_message << std::endl;
			}
		
		//-- If the incoming transaction is a ScratchpadRequest, that means, that this MCPU instructed the VAS Tile to perform a task.
		} else if(std::shared_ptr<ScratchpadRequest> sp_instr_to_schedule = std::dynamic_pointer_cast<ScratchpadRequest>(sched_incoming.front())) {
			std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(sp_instr_to_schedule->getID());
			std::shared_ptr<spike_model::MCPUInstruction> instr_to_schedule = transaction_id->second.mcpu_instruction;
			
			std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_incoming_transaction: " << *sp_instr_to_schedule << ", parent instr: " << *instr_to_schedule << std::endl;

			switch(sp_instr_to_schedule->getCommand()) {
				case ScratchpadRequest::ScratchpadCommand::ALLOCATE:
					sched_outgoing.notify(instr_to_schedule->getDestinationRegId());
					sp_status[instr_to_schedule->getDestinationRegId()] = SPStatus::READY;
					break;
				case ScratchpadRequest::ScratchpadCommand::FREE:
					break;
				case ScratchpadRequest::ScratchpadCommand::READ:
				case ScratchpadRequest::ScratchpadCommand::WRITE:
					computeMemReqAddresses(instr_to_schedule);
					break;
				default:
					sparta_assert(false, "The Memory Tile does not understand this Scratchpad command!");
			}
			
		} else {
			sparta_assert(false, "The incoming packet type is unknown!");
		}
		
		
		//-- consume the instruction from the scheduler
		sched_incoming.pop();
	}
	
	
	//-- Schedule the memory operations going to the MC
	void MemoryCPUWrapper::controllerCycle_mem_requests() {

		//-- Get the oldest Cache Request from the queue
		std::shared_ptr<CacheRequest> instr_for_mc = sched_mem_req.front();
		
		//-- Send to the MC
		out_port_mc_.send(instr_for_mc, 0);

		//-- consume the memory request from the scheduler
		sched_mem_req.pop();
		std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_mem_request: Sending to MC: " << *instr_for_mc << std::endl;
	}
	
	
	void MemoryCPUWrapper::controllerCycle_outgoing_transaction() {
		//-- TODO This if should not be here. The problem is, that if the constructor of MemoryCPUWrapper is called, 
		//        NoC is not yet initialized and hence the NoC leaf in the tree cannot be accessed. Ideally the 
		//        sequence of initialization is changed,
		//		  @see issue #98
		//		  @see MemoryCPUWrapper
		if(noc == nullptr) {
			this->noc = root_node->getChild(std::string("cpu.noc"))->getResourceAs<spike_model::NoC>();	
		}
		
		std::shared_ptr<NoCMessage> response = sched_outgoing.front();	
		if(noc->checkSpaceForPacket(false, response)) {
			out_port_noc_.send(response, 0);
			sched_outgoing.pop();
			std::cout << SPARTA_UNMANAGED_COLOR_GREEN << getClock()->currentCycle() << ": " << name << ": controllerCycle_outgoing_transaction: Sending to NoC: " << *response << SPARTA_UNMANAGED_COLOR_NORMAL << std::endl;
		} else {
			std::cout << SPARTA_UNMANAGED_COLOR_GREEN << getClock()->currentCycle() << ": " << name << ": controllerCycle_outgoing_transaction: NoC does not accept message: " << *response << SPARTA_UNMANAGED_COLOR_NORMAL << std::endl;
		}
	}
	
	
	
	void MemoryCPUWrapper::memOp_unit(std::shared_ptr<MCPUInstruction> instr) {
				
		//-- get the number of elements loaded by 1 memory request
		uint number_of_elements_per_request = line_size_ / (uint32_t)instr->get_width();
		int32_t remaining_elements = vvl_;
		uint64_t address = instr->getAddress();
		
		
		while(remaining_elements > 0) {
			std::cout << getClock()->currentCycle() << ": " << name << ": memOp_unit: noepr: " << number_of_elements_per_request << ", re: " << remaining_elements << ", address: " << address << std::endl;
			
			//-- Generate a cache line request
			std::shared_ptr<CacheRequest> memory_request = createCacheRequest(address, instr);
									
			//-- schedule this request for the MC
			sched_mem_req.push(memory_request);	
			remaining_elements -= number_of_elements_per_request;
			address += line_size_;
		}
		uint32_t number_of_replies = vvl_ / number_of_elements_per_request; // the number of expected CacheRequests returned from the memory controller
		
		std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(instr->getID());
		transaction_id->second.counter_cacheRequests = number_of_replies;		// How many responses are expected from the MC?
		transaction_id->second.counter_scratchpadRequests = number_of_replies;	// How many responses are sent back to the VAS Tile?
		transaction_id->second.number_of_elements_per_response = 1; 			// Send out every received CacheRequest from the MC (no parasitic bytes)
	}
	
	void MemoryCPUWrapper::memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr) {
		
		std::vector<uint64_t> indices = instr->get_index();
		uint64_t address = instr->getAddress();
		
		for(std::vector<uint64_t>::iterator it = indices.begin(); it != indices.end(); ++it) {
			
			//-- create and schedule request for the MC
			std::shared_ptr<CacheRequest> memory_request = createCacheRequest(address + *it, instr); 
			sched_mem_req.push(memory_request);
		}
		
		uint32_t number_of_elements_per_response = line_size_ / (uint32_t)instr->get_width();
		std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(instr->getID());
		transaction_id->second.counter_cacheRequests = vvl_;										// How many responses are expected from the MC?
		transaction_id->second.counter_scratchpadRequests = vvl_/number_of_elements_per_response;	// How many responses are sent back to the VAS Tile?
		transaction_id->second.number_of_elements_per_response = number_of_elements_per_response; 	// How many elements fit into 1 line_size (64 Bytes)?
	}
	
	void MemoryCPUWrapper::memOp_orderedIndex(std::shared_ptr<MCPUInstruction> instr) {
		memOp_nonUnit(instr);	// there is no difference
	}
	
	void MemoryCPUWrapper::memOp_unorderedIndex(std::shared_ptr<MCPUInstruction> instr) {
		memOp_nonUnit(instr);	// there is no difference
	}





	/////////////////////////////////////
	//-- Message handling from the MC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_mc_(const std::shared_ptr<CacheRequest> &mes)	{
		
		if(!enabled) {
			//-- whatever we received from the MC, just forward it to the NoC
			std::cout << getClock()->currentCycle() << ": " << name << ": Returned from MC: " << *mes << std::endl;
			out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile()), 0);
			
			return;
		}
		
		count_requests_mc_++;
		std::cout << getClock()->currentCycle() << ": " << name << ": Returned from MC: instrID: " << mes->getID();
		mes->setServiced();
		
		
		//-- If the ID of the reply is < -, the cache line is for the MCPU and not for the VAS Tile
		if(mes->getID() == 0) {
			std::cout << " - CacheRequest using the Bypass: " << *mes << std::endl;
			std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile());
			sched_outgoing.push(outgoing_noc_message);
		} else {
			std::cout << " - Handled in the MCPU: ";

			std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(mes->getID());
				
			sparta_assert(transaction_id != transaction_table.end(), "Could not find parent instruction!");
			
			uint32_t scratchpadRequest_to_fill = transaction_id->second.counter_cacheRequests % transaction_id->second.number_of_elements_per_response;
			
			transaction_id->second.counter_cacheRequests--;
			
			switch(mes->getType())	{
				case CacheRequest::AccessType::FETCH:
				case CacheRequest::AccessType::LOAD:
					
					if(scratchpadRequest_to_fill == 0) {
						transaction_id->second.counter_scratchpadRequests--;
						
						std::shared_ptr outgoing_message = createScratchpadRequest(mes, ScratchpadRequest::ScratchpadCommand::WRITE);
						outgoing_message->setSize(line_size_);
						outgoing_message->setServiced();
						outgoing_message->setOperandReady(transaction_id->second.counter_scratchpadRequests == 0);
						
						std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(outgoing_message, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, getID(), transaction_id->second.mcpu_instruction->getSourceTile());
						
						
						//-- Check if the SP is already allocated. If not, store the msg in a delaying queue
						size_t destReg = transaction_id->second.mcpu_instruction->getDestinationRegId();
						if(sp_status[destReg] == SPStatus::READY) {
							sched_outgoing.push(noc_message);
						} else {
							sched_outgoing.add_delay_queue(noc_message, destReg);
						}
						
						std::cout << "Returning SP: " << *outgoing_message;
					}
					break;
				case CacheRequest::AccessType::WRITEBACK:
				case CacheRequest::AccessType::STORE:
					//-- TODO: Does an ACK(?) be sent back, once the operation is complete?
					break;
			}
			
			std::cout << ", SP to fill: " << scratchpadRequest_to_fill << ", CRs/SPRs to go: " << transaction_id->second.counter_cacheRequests << "/" << transaction_id->second.counter_scratchpadRequests;
			
			if(transaction_id->second.counter_cacheRequests == 0) { //Counting the number of the CacheRequest-Replies
					transaction_table.erase(transaction_id); //if the counter reaches 0, remove MCPUInstr from hash table.
					std::cout << " - complete";
			}
			std::cout << std::endl;
		}
	}



	/////////////////////////////////////
	//-- Helper functions
	/////////////////////////////////////
	std::shared_ptr<ScratchpadRequest> MemoryCPUWrapper::createScratchpadRequest(const std::shared_ptr<Request> &mes, ScratchpadRequest::ScratchpadCommand command) {
		std::shared_ptr<ScratchpadRequest> sp_request = std::make_shared<ScratchpadRequest>(
						mes->getAddress(),
						command,
						mes->getPC(),
						mes->getTimestamp(),
						mes->getCoreId(),
						getID(),
						mes->getDestinationRegId()
		);
		sp_request->setID(mes->getID());
		return sp_request;
	}
	
	
	std::shared_ptr<CacheRequest> MemoryCPUWrapper::createCacheRequest(uint64_t address, std::shared_ptr<MCPUInstruction> instr) {
		std::shared_ptr<CacheRequest> cr = std::make_shared<CacheRequest>(
					address,
					(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
								CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
					instr->getPC(),
					instr->getTimestamp(), 
					instr->getCoreId());

		cr->setDestinationReg(instr->getDestinationRegId(), instr->getDestinationRegType());
		cr->setID(instr->getID());	// set CacheRequest ID to the mcpu instruction ID it was generated from		
		
		return cr;
	}
	
	
	void MemoryCPUWrapper::computeMemReqAddresses(std::shared_ptr<MCPUInstruction> instr) {
		switch(instr->get_suboperation()) {
			case MCPUInstruction::SubOperation::UNIT:
				memOp_unit(instr);
				break;
			case MCPUInstruction::SubOperation::NON_UNIT:
				memOp_nonUnit(instr);
				break;
			case MCPUInstruction::SubOperation::ORDERED_INDEX:
				memOp_orderedIndex(instr);
				break;
			case MCPUInstruction::SubOperation::UNORDERED_INDEX:
				memOp_unorderedIndex(instr);
				break;
			default:
				sparta_assert(false, "UNKNOWN MCPUInstruction SubOperation. The MCPU does not understand, what that instruction means.");
		}
	}


	void MemoryCPUWrapper::notifyCompletion_() {}
	

	void MemoryCPUWrapper::setRequestManager(std::shared_ptr<EventManager> r) {
		request_manager_ = r;
	}
}


// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab: