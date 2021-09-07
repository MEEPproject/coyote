#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryCPUWrapper.hpp"

//-- Activate debugging messages for the Memory Tile
//#define DEBUG_MEMTILE

#ifdef DEBUG_MEMTILE
	#define DEBUG_MSG(str) do {std::cout << getClock()->currentCycle() << ": " << name << ": " << __func__ << ": " << str << std::endl;} while(0)
	#define DEBUG_MSG_COLOR(color, str) do {std::cout << color; DEBUG_MSG(str); std::cout << SPARTA_UNMANAGED_COLOR_NORMAL;} while(0)
#else
	#define DEBUG_MSG(str) do {} while(0)
	#define DEBUG_MSG_COLOR(color, str) DEBUG_MSG(str)
#endif

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
			sched_outgoing(&controller_cycle_event_outgoing_transaction, p->latency) {
				in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_noc_, std::shared_ptr<NoCMessage>));
				in_port_mc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_mc_, std::shared_ptr<CacheRequest>));
				instructionID_counter = 1;
				this->id = 0;
								
				sp_status = (SPStatus *)calloc(num_of_registers, sizeof(SPStatus));
				sparta_assert(sp_status != nullptr, "SP status array could not be allocated!");
								
				//-- read the configuration
				root_node			= node->getRoot()->getAs<sparta::RootTreeNode>();
				auto config			= root_node->getSimulator()->getSimulationConfiguration()->getUnboundParameterTree();
				this->enabled		= config.tryGet("meta.params.enable_smart_mcpu")->getAs<bool>();
				this->sp_reg_size	= (size_t)config.tryGet("top.cpu.tile0.params.num_l2_banks")->getAs<uint16_t>() *
									  (size_t)config.tryGet("top.cpu.tile0.l2_bank0.params.size_kb")->getAs<uint64_t>() *
									  (size_t)1024 / (
									  	(size_t)config.tryGet("top.cpu.params.num_cores")->getAs<uint16_t>() /
									  	(size_t)config.tryGet("top.cpu.params.num_tiles")->getAs<uint16_t>()
									  ) /
									  (size_t)num_of_registers;
				
				
				DEBUG_MSG("Memory Tile is " << (this->enabled ? "enabled" : "disabled") << ".");					  
				if(enabled) {
					DEBUG_MSG("Each register entry in the SP is " << sp_reg_size << " Bytes.");
				}
				
				this->noc      = nullptr;
	}


	/////////////////////////////////////
	//-- Message handling from the NoC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes) {
		DEBUG_MSG("Received from NoC: " << *mes);
		if(enabled) {
			count_requests_noc_++;
		}
		
		mes->getRequest()->handle(this);
	}


	//-- Bypass for a scalar memory operation
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::CacheRequest> mes) {
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_CYAN, "CacheRequest: " << *mes);
		if(!enabled) {
			//-- whatever comes into the Memory Tile, just send it out to the MC
			out_port_mc_.send(mes, 0);
			return;
		}
			
			if(mes->getMemTile() == (uint16_t)-1) {			//-- a transaction from the VAS Tile
				
				uint16_t destMemTile = calcDestMemTile(mes->getAddress());
				if(destMemTile == getID()) {	//-- the address range is served by the current memory tile
					//-- Schedule memory request for the MC
					sched_mem_req.push(mes);
				} else {						//-- the address range is not served by the current memory tile
					mes->setMemTile(getID());
					std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(
							mes,
							NoCMessageType::MEM_TILE_REQUEST,
							line_size_,
							destMemTile,
							getID()
					);
					sched_outgoing.push(noc_message);
					DEBUG_MSG("\tForwarding to Memory Tile " << destMemTile);
			}
		} else {
			if(mes->isServiced()) {					//-- this transaction has been completed by a different memory tile.
				handleReplyMessage(mes);
			} else {								//-- the parent transaction has been received by a different memory tile, but it is served here.
				DEBUG_MSG("\tSource is a different Memory Tile: " << mes->getMemTile());
				sched_mem_req.push(mes);
			}
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
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN, "MCPUSetVVL: " << *mes);
			
		std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, line_size_, getID(), mes->getSourceTile());
		sched_outgoing.push(outgoing_noc_message);
	}



	//-- A vector instruction for the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUInstruction> instr) {
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle an MCPUInstruction!");
		
		instr->setID(this->instructionID_counter);
		struct transaction instruction_attributes = {instr, 0, 0, 1};
		transaction_table.insert({this->instructionID_counter, instruction_attributes}); // keep track of the transaction
		this->instructionID_counter++; // increment ID
		if(this->instructionID_counter == 0) {this->instructionID_counter = 1;}	// handle the overflow. 0 is reserved for cache requests that use the bypass
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN, "Memory instruction received: " << *instr);

		
		if(instr->get_operation() == MCPUInstruction::Operation::LOAD) {
			
			//-- If we have not done before, an ALLOCATE has to be sent to the VAS Tile, so it reserves space for the data to be written to the SP
			if(sp_status[instr->getDestinationRegId()] == SPStatus::IS_L2) {
				sp_status[instr->getDestinationRegId()] = SPStatus::ALLOC_SENT;
				
				std::shared_ptr<ScratchpadRequest> sp_request = createScratchpadRequest(instr, ScratchpadRequest::ScratchpadCommand::ALLOCATE);
				sp_request->setSize(vvl * (uint)instr->get_width());	// reserve the space in the VAS Tile
				std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(sp_request, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, getID(), instr->getSourceTile());
				sched_outgoing.push(noc_message);
				
				DEBUG_MSG("sending SP ALLOC: " << *sp_request);
			}
			
			//-- Send the request to the Vector Address Generators (VAG)
			computeMemReqAddresses(instr);
			
		} else { // If it is a store, generate a ScratchpadRequest to load the data from the VAS Tile
			
			std::shared_ptr outgoing_message = createScratchpadRequest(instr, ScratchpadRequest::ScratchpadCommand::READ);
				outgoing_message->setSize(line_size_);
			outgoing_message->setOperandReady(true);
			
			
			std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(outgoing_message, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, getID(), instr->getSourceTile());
			
			//-- TODO issue #94 needs to be fixed first
			//sched_outgoing.push(noc_message);
			
			DEBUG_MSG(", sending SP READ: " << *outgoing_message);
		}
	}


	//-- Handle for Scratchpad requests
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::ScratchpadRequest> instr) {
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle a ScratchpadRequest!");
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN, "ScratchpadRequest: " << *instr);
		
		std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(instr->getID());
		std::shared_ptr<spike_model::MCPUInstruction> parent_instr = transaction_id->second.mcpu_instruction;
		
		DEBUG_MSG(*instr << ", parent instr: " << *parent_instr);

		switch(instr->getCommand()) {
			case ScratchpadRequest::ScratchpadCommand::ALLOCATE:
				sched_outgoing.notify(parent_instr->getDestinationRegId());
				sp_status[parent_instr->getDestinationRegId()] = SPStatus::READY;
				break;
			case ScratchpadRequest::ScratchpadCommand::FREE:
				break;
			case ScratchpadRequest::ScratchpadCommand::READ:
				std::cout << "Got a read. NOT YET IMPLEMENTED";
				break;
			case ScratchpadRequest::ScratchpadCommand::WRITE:
				computeMemReqAddresses(parent_instr);
				break;
			default:
				sparta_assert(false, "The Memory Tile does not understand this Scratchpad command!");
		}
	}

	
	
	//-- Schedule the memory operations going to the MC
	void MemoryCPUWrapper::controllerCycle_mem_requests() {

		//-- Get the oldest Cache Request from the queue
		std::shared_ptr<CacheRequest> instr_for_mc = sched_mem_req.front();
		
		//-- Send to the MC
		out_port_mc_.send(instr_for_mc, 1);

		//-- consume the memory request from the scheduler
		sched_mem_req.pop();
		DEBUG_MSG("Sending to MC: " << *instr_for_mc);
	}
	
	
	void MemoryCPUWrapper::controllerCycle_outgoing_transaction() {

		sparta_assert(noc, "The MCPU does not have access to methods of the NoC, since MemoryCPUWrapper::setNoC(spike_model::NoC) was not called.");
		
		std::shared_ptr<NoCMessage> response = sched_outgoing.front();	
		if(noc->checkSpaceForPacket(false, response)) {
			out_port_noc_.send(response, 0);
			sched_outgoing.pop();
			DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_GREEN, "Sending to NoC: " << *response);
		} else {
			DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_GREEN, "NoC does not accept message: " << *response);

			//-- reschedule for the next cycle
			sched_outgoing.reschedule();
		}
	}
	
	
	
	void MemoryCPUWrapper::memOp_unit(std::shared_ptr<MCPUInstruction> instr) {
				
		//-- get the number of elements loaded by 1 memory request
		uint number_of_elements_per_request = line_size_ / (uint32_t)instr->get_width();
		int32_t remaining_elements = vvl_;
		uint64_t address = instr->getAddress();
		
		
		while(remaining_elements > 0) {
			DEBUG_MSG("memOp_unit: noepr: " << number_of_elements_per_request << ", re: " << remaining_elements << ", address: " << address);
			
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
			DEBUG_MSG("Returned from MC: " << *mes);
			out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile()), 0);
			
			return;
		}
		
		count_requests_mc_++;
		mes->setServiced();
		
		
		DEBUG_MSG("Returned from MC: instrID: " << mes->getID());

		//-- If the ID of the reply is < 0, the cache line is for the MCPU and not for the VAS Tile
		//   If the ID is = 0, the CacheRequest utilizes the Bypass (scalar operation)
		//   If the ID is > 0, the CacheRequest is to be handled in this Memory Tile
		if(mes->getID() == 0) {
			DEBUG_MSG("\tCacheRequest using the Bypass: " << *mes);
			std::shared_ptr<NoCMessage> outgoing_noc_message;
			if(mes->getMemTile() == (uint16_t)-1) {
				outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile());	
			} else {
				outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEM_TILE_REPLY, line_size_, getID(), mes->getMemTile());
			}
			
			sched_outgoing.push(outgoing_noc_message);
		} else {
			handleReplyMessage(mes);
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
            
	void MemoryCPUWrapper::setAddressMappingInfo(uint64_t memory_controller_shift, uint64_t memory_controller_mask) {
		mc_shift = memory_controller_shift;
		mc_mask  = memory_controller_mask;
	}

	uint16_t MemoryCPUWrapper::calcDestMemTile(uint64_t address) {
		return (uint16_t)((address >> mc_shift) & mc_mask);
	}
	
	void MemoryCPUWrapper::handleReplyMessage(std::shared_ptr<CacheRequest> mes) {
		DEBUG_MSG("\tHandled in the MCPU:");

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
					
					DEBUG_MSG("\t\tReturning SP: " << *outgoing_message);
				}
				break;
			case CacheRequest::AccessType::WRITEBACK:
			case CacheRequest::AccessType::STORE:
				//-- TODO: Does an ACK(?) be sent back, once the operation is complete?
				break;
		}
		
		DEBUG_MSG("\t\tSP to fill: " << scratchpadRequest_to_fill << ", CRs/SPRs to go: " << transaction_id->second.counter_cacheRequests << "/" << transaction_id->second.counter_scratchpadRequests);
		
		if(transaction_id->second.counter_cacheRequests == 0) { //Counting the number of the CacheRequest-Replies
				transaction_table.erase(transaction_id); //if the counter reaches 0, remove MCPUInstr from hash table.
				DEBUG_MSG("\t\t\tcomplete");
		}
	}
	
	void MemoryCPUWrapper::setID(uint16_t new_id) {
		auto config	= root_node->getSimulator()->getSimulationConfiguration()->getUnboundParameterTree();
		uint16_t max_id = config.tryGet("top.cpu.params.num_memory_cpus")->getAs<uint16_t>();
		
		sparta_assert(new_id < max_id, "The given ID for the Memory Tile is out of range!");
		
		this->id = new_id;
	}
	
	uint16_t MemoryCPUWrapper::getID() {
		return this->id;
	}
}



// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab: