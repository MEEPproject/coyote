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
			sched_mem_req(&controller_cycle_event_mem_req),
			sched_outgoing(&controller_cycle_event_outgoing_transaction),
			sched_incoming(&controller_cycle_event_incoming_transaction)
			{
				in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_noc_, std::shared_ptr<NoCMessage>));
				in_port_mc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_mc_, std::shared_ptr<CacheRequest>));
				instructionID_counter = 1;
				set_id(0);
			}
            
			

	/////////////////////////////////////
	//-- Message handling from the NoC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes) {
		std::cout << getClock()->currentCycle() << ": " << name << ": receiveMessage_noc_: Received from NoC: " << *mes << std::endl;
		
		count_requests_noc_++;
		mes->getRequest()->handle(this);
	}


	//-- Bypass for a scalar memory operation
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::CacheRequest> mes) {
		std::cout << getClock()->currentCycle() << ": " << name << ": Instruction for the MC received: " << *mes << std::endl;
		switch(mes->getType()) {
			case CacheRequest::AccessType::FETCH:
			case CacheRequest::AccessType::LOAD:
				count_load_++;
				//std::cout << "Loading: " << mes << ", coreID: " << mes->getCoreId() << std::endl;
				break;
			case CacheRequest::AccessType::WRITEBACK:
			case CacheRequest::AccessType::STORE:
				count_store_++;
				//std::cout << "Storing: " << mes << ", coreID: " << mes->getCoreId() << std::endl;
				break;
		}
		
		//-- Schedule memory request for the MC
		sched_mem_req.push(mes);
	}

	//-- A memory transaction to be handled by the MCPU
	//   Note that VVL of 0 is received when the simulation is setting up
	//   and the processor objects are created.
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUSetVVL> mes) {
		std::cout <<  getClock()->currentCycle() << ": " << name << ": handle MCPUSetVVL: " << *mes << std::endl;

			//-- TODO: Compute AVL using some more reasonable value.
			vvl_ = mes->getAVL()/2;
			mes->setVVL(vvl_);
			
			mes->setServiced();
			
			//   Thread ID is not required currently
			std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, line_size_, this->id, mes->getSourceTile());
			sched_outgoing.push(outgoing_noc_message);
	}

	//-- A vector instruction for the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUInstruction> r) {
		std::cout << getClock()->currentCycle() << ": " << name << ": handle: Memory instruction received: " << *r << std::endl;
		
		r->setMCPUInstruction_ID(this->instructionID_counter);
		
		struct transaction instruction_attributes = {r, 0, 0, 1};
		transaction_table.insert({this->instructionID_counter, instruction_attributes}); // keep track of the transaction
		this->instructionID_counter++; // increment ID
		if(this->instructionID_counter == 0) this->instructionID_counter = 1;	// handle the overflow. 0 is reserved for cache requests that use the bypass
		
		//-- schedule the incoming message
		sched_incoming.push(r);
	}

    
	void MemoryCPUWrapper::controllerCycle_incoming_transaction() {
		std::shared_ptr<MCPUInstruction> instr_to_schedule = sched_incoming.front();
		std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_incoming_transaction: " << *instr_to_schedule << std::endl;
		
		switch(instr_to_schedule->get_suboperation()) {
			case MCPUInstruction::SubOperation::UNIT:
				memOp_unit(instr_to_schedule);
				break;
			case MCPUInstruction::SubOperation::NON_UNIT:
				memOp_nonUnit(instr_to_schedule);
				break;
			case MCPUInstruction::SubOperation::ORDERED_INDEX:
				memOp_orderedIndex(instr_to_schedule);
				break;
			case MCPUInstruction::SubOperation::UNORDERED_INDEX:
				memOp_unorderedIndex(instr_to_schedule);
				break;
			default:
				std::cerr << getClock()->currentCycle() << ": " << name << ": UNKNOWN MCPUInstruction SubOperation. The MCPU does not understand, what that instruction means. Ignoring it." << std::endl;
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
	
	
	void MemoryCPUWrapper::controllerCycle_outgoing_transaction() {
	  	std::shared_ptr<NoCMessage> response = sched_outgoing.front();	
		out_port_noc_.send(response, 0);
		sched_outgoing.pop();
		std::cout << getClock()->currentCycle() << ": " << name << ": controllerCycle_outgoing_transaction: Sending to NoC: " << *response << std::endl;
	}
	
	
	
	void MemoryCPUWrapper::memOp_unit(std::shared_ptr<MCPUInstruction> instr) {
				
		//-- get the number of elements loaded by 1 memory request
		uint number_of_elements_per_request = line_size_ / (uint32_t)instr->get_width();
		uint32_t remaining_elements = vvl_;
		uint64_t address = instr->get_baseAddress();
		
		
		while(remaining_elements > 0) {
			//-- Generate a cache line request
			std::shared_ptr<CacheRequest> memory_request = std::make_shared<CacheRequest>(
						address,
						(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
									CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
						0,
						getClock()->currentCycle(), 
						get_id());

		    memory_request->setParentInstruction_ID(instr->getMCPUInstruction_ID());	// set cacherequest ID to the mcpu instruction ID it was generated from		
			
			
			//-- schedule this request for the MC
			sched_mem_req.push(memory_request);	
			remaining_elements -= number_of_elements_per_request;
			address += line_size_;
		}
		uint32_t number_of_replies = vvl_ / number_of_elements_per_request; // the number of expected CacheRequests returned from the memory controller
		
		std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(instr->getMCPUInstruction_ID());
		transaction_id->second.counter_cacheRequests = number_of_replies;		// How many responses are expected from the MC?
		transaction_id->second.counter_scratchpadRequests = number_of_replies;	// How many responses are sent back to the VAS Tile?
		transaction_id->second.number_of_elements_per_response = 1; 			// Send out every received CacheRequest from the MC (no parasitic bytes)

	}
	
	void MemoryCPUWrapper::memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr) {
		
		std::vector<uint64_t> indices = instr->get_index();
		uint64_t address = instr->get_baseAddress();
		for(std::vector<uint64_t>::iterator it = indices.begin(); it != indices.end(); ++it) {
            std::shared_ptr<CacheRequest> mem_op = std::make_shared<CacheRequest>(
						(address + *it),
						(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
									CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
						0,
						getClock()->currentCycle(), 
						get_id()); 

			mem_op->setParentInstruction_ID(instr->getMCPUInstruction_ID());	// set cacherequest ID to the mcpu instruction ID it was generated from			
            
			//-- schedule request for the MC
            sched_mem_req.push(mem_op);
		}
		
		uint32_t number_of_elements_per_response = line_size_ / (uint32_t)instr->get_width();
		std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(instr->getMCPUInstruction_ID());
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
		count_requests_mc_++;
		std::cout << getClock()->currentCycle() << ": " << name << ": Returned from MC: " << *mes << " instrID: " << mes->getParentInstruction_ID();
		mes->setServiced();
	    
		
		//-- If the ID of the reply is < -, the cache line is for the MCPU and not for the VAS Tile
		if(mes->getParentInstruction_ID() == 0) {
			std::cout << " - CacheRequest using the Bypass" << std::endl;
			std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile());
            sched_outgoing.push(outgoing_noc_message);
		} else {
			std::cout << " - Handled in the MCPU" << std::endl; 

			std::unordered_map<std::uint32_t, transaction>::iterator transaction_id = transaction_table.find(mes->getParentInstruction_ID());
			transaction_id->second.counter_cacheRequests--;
			
			uint32_t scratchpadRequest_to_fill = transaction_id->second.counter_cacheRequests % transaction_id->second.number_of_elements_per_response;
       
			if(scratchpadRequest_to_fill == 0) {
				transaction_id->second.counter_scratchpadRequests--;
	        	
				std::shared_ptr<ScratchpadRequest> outgoing_request = std::make_shared<ScratchpadRequest>(
						mes->getAddress(),
						(mes->getType() == CacheRequest::AccessType::LOAD) ? 
									ScratchpadRequest::ScratchpadCommand::WRITE : ScratchpadRequest::ScratchpadCommand::READ,
						mes->getPC(),
						getClock()->currentCycle(), 
						transaction_id->second.counter_scratchpadRequests == 0);
				
				std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(outgoing_request, NoCMessageType::SCRATCHPAD_COMMAND, line_size_, get_id(), transaction_id->second.mcpu_instruction->getSourceTile());
				sched_outgoing.push(outgoing_noc_message);
				
				if(transaction_id->second.counter_scratchpadRequests == 0) {
					// delete from hash table
					transaction_table.erase(transaction_id);
				}
			}
		}
	}


	void MemoryCPUWrapper::notifyCompletion_() {
	}

	void MemoryCPUWrapper::setRequestManager(std::shared_ptr<EventManager> r) {
		request_manager_ = r;
	}
}
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab:

