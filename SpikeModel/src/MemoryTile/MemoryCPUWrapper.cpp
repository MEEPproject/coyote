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
				
				auto root_node = node->getRoot()->getAs<sparta::RootTreeNode>()->getSimulator()->getSimulationConfiguration()->getUnboundParameterTree();
				this->enabled = root_node.tryGet("meta.params.enable_smart_mcpu")->getAs<bool>();
				std::cout << "Memory Tile is " << (this->enabled ? "enabled" : "disabled") << "." << std::endl;
			}


	/////////////////////////////////////
	//-- Message handling from the NoC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes) {
		if(enabled) {
			std::cout << getClock()->currentCycle() << ": " << name << ": receiveMessage_noc_: Received from NoC: " << *mes << std::endl;
			count_requests_noc_++;
		}
		
		mes->getRequest()->handle(this);
	}


	//-- Bypass for a scalar memory operation
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::CacheRequest> mes) {
		
		if(enabled) {
			//-- Schedule memory request for the MC
			sched_mem_req.push(mes);
			std::cout << getClock()->currentCycle() << ": " << name << ": Instruction for the MC received: " << *mes << std::endl;
		} else {
			//-- whatever comes into the Memory Tile, just send it out to the MC
			out_port_mc_.send(mes, 0);
		}
	}

	//-- A memory transaction to be handled by the MCPU
	//   Note that VVL of 0 is received when the simulation is setting up
	//   and the processor objects are created.
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUSetVVL> mes) {
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle an MCPUSetVVL instruction!");

		//-- TODO: Compute AVL using some more reasonable value.
		//		   Thread ID is not required currently
		vvl_ = mes->getAVL()/2;
		mes->setVVL(vvl_);
			
		mes->setServiced();
		
		std::cout <<  getClock()->currentCycle() << ": " << name << ": handle MCPUSetVVL: " << *mes << std::endl;
			
		std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, line_size_, getID(), mes->getSourceTile());
		sched_outgoing.push(outgoing_noc_message);
	}


	//-- A vector instruction for the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUInstruction> r){
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle an MCPUInstruction!");
		
		std::cout << getClock()->currentCycle() << ": " << name << ": handle: Memory instruction received: " << *r << std::endl;
		
		r->setID(this->instructionID_counter);
		
		struct transaction instruction_attributes = {r, 0, 0, 1};
		transaction_table.insert({this->instructionID_counter, instruction_attributes}); // keep track of the transaction
		this->instructionID_counter++; // increment ID
		if(this->instructionID_counter == 0) {this->instructionID_counter = 1;}	// handle the overflow. 0 is reserved for cache requests that use the bypass
		
		//-- schedule the incoming message
		sched_incoming.push(r);
	}


	//-- Handle for Scratchpad requests
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::ScratchpadRequest> c){
		
		sparta_assert(enabled, "The Memory Tile needs to be enabled to handle a ScratchpadRequest!");
		
		std::cout << getClock()->currentCycle() << ": " << name << ": handle: ScratchpadRequest: " << *c << std::endl;
		sched_incoming.push(c);
	}





	void MemoryCPUWrapper::controllerCycle_incoming_transaction() {
		
		//-- If the incoming transaction is a MCPU instruction, that means, that the VAS Tile instructs the MCPU to perform an operation.
		if(std::shared_ptr<MCPUInstruction> instr_to_schedule = std::dynamic_pointer_cast<MCPUInstruction>(sched_incoming.front())) {
			std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_incoming_transaction: " << *instr_to_schedule << std::endl;
			
			//-- If it is a load
			if(instr_to_schedule->get_operation() == MCPUInstruction::Operation::LOAD) {
			
				computeMemReqAddresses(instr_to_schedule);
				
			} else { // If it is a store, generate a ScratchpadRequest to load the data from the VAS Tile
				
				std::shared_ptr outgoing_message = createScratchpadRequest(instr_to_schedule, ScratchpadRequest::ScratchpadCommand::READ);
				outgoing_message->setSize(line_size_);
				outgoing_message->setOperandReady();
				
				
				std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(outgoing_message, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, getID(), instr_to_schedule->getSourceTile());
				sched_outgoing.push(noc_message);
			}
		
		//-- If the incoming transaction is a ScratchpadRequest, that means, that this MCPU instructed the VAS Tile to perform a task. In this case it
		//   is always a vector store operation.
		} else if(std::shared_ptr<ScratchpadRequest> sp_instr_to_schedule = std::dynamic_pointer_cast<ScratchpadRequest>(sched_incoming.front())) {
			std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(sp_instr_to_schedule->getID());
			std::shared_ptr<spike_model::MCPUInstruction> instr_to_schedule = transaction_id->second.mcpu_instruction;

			computeMemReqAddresses(instr_to_schedule);
			
		} else {
			sparta_assert(false, "The incoming packet type is unknown!");
		}
		
		
		//-- consume the instruction from the scheduler
		sched_incoming.pop();
	}
	
	
	//-- Schedule the memory operations going to mc
	void MemoryCPUWrapper::controllerCycle_mem_requests() {

		//-- Get the oldest Cache Request from the queue
		std::shared_ptr<CacheRequest> instr_for_mc = sched_mem_req.front();
		
		//-- Send to the MC
		out_port_mc_.send(instr_for_mc, 0);

		//-- consume the memory request from the scheduler
		sched_mem_req.pop();
		std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_mem_request: Sending to MC: " << *instr_for_mc << std::endl;
	}
	
	
	void MemoryCPUWrapper::controllerCycle_outgoing_transaction(){
		std::shared_ptr<NoCMessage> response = sched_outgoing.front();	
		out_port_noc_.send(response, 0);
		sched_outgoing.pop();
		std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_outgoing_transaction: Sending to NoC: " << *response << std::endl;
	}
	
	
	
	void MemoryCPUWrapper::memOp_unit(std::shared_ptr<MCPUInstruction> instr) {
				
		//-- get the number of elements loaded by 1 memory request
		uint number_of_elements_per_request = line_size_ / (uint32_t)instr->get_width();
		uint32_t remaining_elements = vvl_;
		uint64_t address = instr->getAddress();
		
		
		while(remaining_elements > 0) {
			std::cout << getClock()->currentCycle() << ": " << name << "memOp_unit: noepr: " << number_of_elements_per_request << ", re: " << remaining_elements << ", address: " << address << std::endl;
			//-- Generate a cache line request
			std::shared_ptr<CacheRequest> memory_request = std::make_shared<CacheRequest>(
						address,
						(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
									CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
						0,
						getClock()->currentCycle(), 
						getID());

			memory_request->setID(instr->getID());	// set cacherequest ID to the mcpu instruction ID it was generated from		
			
			
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
		
		std::cout << "memOp_unit: noepr: " << number_of_elements_per_request << ", re: " << remaining_elements << ", address: " << address << ", nor: " << number_of_replies << std::endl;
		
	}
	
	void MemoryCPUWrapper::memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr) {
		
		std::vector<uint64_t> indices = instr->get_index();
		uint64_t address = instr->getAddress();
		for(std::vector<uint64_t>::iterator it = indices.begin(); it != indices.end(); ++it) {
			std::shared_ptr<CacheRequest> mem_op = std::make_shared<CacheRequest>(
						(address + *it),
						(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
									CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
						0,
						getClock()->currentCycle(), 
						getID()); 

			mem_op->setID(instr->getID());	// set cacherequest ID to the mcpu instruction ID it was generated from			
			
			//-- schedule request for the MC
			sched_mem_req.push(mem_op);
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
			out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile()), 0);
			
			return;
		}
		
		count_requests_mc_++;
		std::cout << getClock()->currentCycle() << ": " << name << ": Returned from MC: " << *mes << " instrID: " << mes->getID();
		mes->setServiced();
		
		
		//-- If the ID of the reply is < -, the cache line is for the MCPU and not for the VAS Tile
		if(mes->getID() == 0) {
			std::cout << " - CacheRequest using the Bypass" << std::endl;
			std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile());
			sched_outgoing.push(outgoing_noc_message);
		} else {
			std::cout << " - Handled in the MCPU";

			std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(mes->getID());
			
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
						if(transaction_id->second.counter_scratchpadRequests == 0) {
							outgoing_message->setOperandReady();
						}
						
						std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(outgoing_message, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, getID(), transaction_id->second.mcpu_instruction->getSourceTile());
						sched_outgoing.push(noc_message);
						
						if(transaction_id->second.counter_scratchpadRequests == 0) {
							// delete from hash table
							transaction_table.erase(transaction_id);
							std::cout << " - complete";
						}
					}
					break;
				case CacheRequest::AccessType::WRITEBACK:
				case CacheRequest::AccessType::STORE:
					if(transaction_id->second.counter_cacheRequests == 0) { //Counting the number of the CacheRequest-Replies
						transaction_table.erase(transaction_id); //if the counter reaches 0, remove MCPUInstr from hash table.
						std::cout << " - complete";
					}
					break;
			}
			std::cout << ", SP to fill: " << scratchpadRequest_to_fill << ", CRs/SPRs to go: " << transaction_id->second.counter_cacheRequests << "/" << transaction_id->second.counter_scratchpadRequests;
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
						getClock()->currentCycle(),
						mes->getCoreId(),
						getID(),
						mes->getDestinationRegId()
		);
		sp_request->setID(mes->getID());
		return sp_request;
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