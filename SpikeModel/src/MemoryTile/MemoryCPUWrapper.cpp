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
			sched_incoming(&controller_cycle_event_incoming_transaction)
			{
				in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_noc_, std::shared_ptr<NoCMessage>));
				in_port_mc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_mc_, std::shared_ptr<CacheRequest>));
			}
            
			

	/////////////////////////////////////
	//-- Message handling from the NoC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes) {
		count_requests_noc_++;
		mes->getRequest()->handle(this);
	}


	//-- Bypass for a scalar memory operation
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::CacheRequest> mes) {
		//std::cout << "MCPU: Instruction for the MC received" << std::endl;
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
		std::cout <<  "  VVL " << mes->getVVL() << " received from MCPU: Core "
						<< mes->getCoreId() <<std::endl;

			//-- TODO: Compute AVL
			vvl_ = mes->getAVL();
			mes->setVVL(vvl_);
			
			//   Thread ID is not required currently
			//-- TODO: Return the new VVL
	}

	//-- A vector instruction for the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUInstruction> r) {
		std::cout << "MCPU: Memory instruction received. Core: " << r->getCoreId() << ", BaseAddress: " << r->get_baseAddress() << ", Size: "
					<< (int)r->get_width() << ", op: " << (int)r->get_operation() << ", sub_op: " << (int)r->get_suboperation() << std::endl;
		
		//-- schedule the incoming message
		sched_incoming.push(r);
	}

    
	void MemoryCPUWrapper::controllerCycle_incoming_transaction(){
		std::shared_ptr<MCPUInstruction> instr_to_schedule = sched_incoming.front();
		
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
				std::cerr << "UNKNOWN MCPUInstruction SubOperation. The MCPU does not understand, what that instruction means. Ignoring it." << std::endl;
		}
		
		//-- consume the instruction from the scheduler
		sched_incoming.pop();
	}
	
	void MemoryCPUWrapper::controllerCycle_mem_requests() {
		// Schedule the memory operations going to mc

		//-- Get the oldest Cache Request from the queue
		std::shared_ptr<CacheRequest> instr_for_mc = sched_mem_req.front();
		
		//-- Send to the MC
		out_port_mc_.send(instr_for_mc, 0);

        //-- consume the memory request from the scheduler
		sched_mem_req.pop();
	}
	
	
	void MemoryCPUWrapper::schedule_outgoing_mem_ops() {
		
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
						(uint16_t)-1);
			
			//-- schedule this request for the MC
			sched_mem_req.push(memory_request);	
			remaining_elements -= number_of_elements_per_request;
			address += line_size_;
		}
	}
	
	void MemoryCPUWrapper::memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr) {
		
		std::vector<uint64_t> indices = instr->get_index();
		uint64_t address = instr->get_baseAddress();
		for(std::vector<uint64_t>::iterator it = indices.begin(); it != indices.end(); ++it) {
			//-- TODO Generate cache line requests
			// The following code is incorrect. @Regina: Have a look at the method "memOp_unit", which handles
			// unit stride vector memory operations (unit stride = all vector elements are next to each other). This is
			// the non-unit stride vector memory operation, in which the elements are handled in regular intervals. E.g.,
			// load every second, third,... element.
			// In Coyote we handle the non-unit stride vector operations a bit differently: The MCPU instruction contains
			// a field with indices. So for e.g. a non-unit stride vector load with a stride of 2, the field contains
			// 0, 2, 4, 6, 8, 10,... (VVL times). Hence, there is no difference in unordered indexed memory operations (methods below)
			
			
			// So here is the incorrect code:
			//std::shared_ptr<MCPUInstruction> mem_op = std::make_shared<MCPUInstruction>(*instr);
			//
			//mem_op->set_baseAddress(instr->get_baseAddress() + *it);
			//sched_mem_req.push(mem_op);

			//
            std::shared_ptr<CacheRequest> mem_op = std::make_shared<CacheRequest>(
						(address + *it),
						(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
									CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
						0,
						getClock()->currentCycle(), 
						(uint16_t)-1); 
            
			//-- schedule request for the MC
             sched_mem_req.push(mem_op);

		}
	}
	
	void MemoryCPUWrapper::memOp_orderedIndex(std::shared_ptr<MCPUInstruction> instr) {
		memOp_nonUnit(instr);	// there is no difference
	}
	
	void MemoryCPUWrapper::memOp_unorderedIndex(std::shared_ptr<MCPUInstruction> instr) {
		memOp_nonUnit(instr);	// there is no difference
	}


	/////////////////////////////////////
	//-- Command Execution
	/////////////////////////////////////
	/*void MemoryCPUWrapper::issueMCPU_() {
		std::shared_ptr<MCPUSetVVL> mes = mcpu_req.front();
		mes->setReturnedVecLen(mes->getRequestedVecLen());
		std::cout << "MCPU: Returning VVL " << mes->getReturnedVecLen() << " to core " << mes->getCoreId() << std::endl;
		mes->setServiced();
		out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, line_size_, mes->getMemoryCPU(), mes->getSourceTile()), 0);

		//-- Are there any messages left in the queue?
		mcpu_req.pop_front();
		if(mcpu_req.size() > 0) {
			issue_mcpu_event_.schedule(1);
		}
	}*/




	/////////////////////////////////////
	//-- Message handling from the MC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_mc_(const std::shared_ptr<CacheRequest> &mes)	{
		count_requests_mc_++;
		std::cout << "Returning: " << mes << ", coreID: " << mes->getCoreId() << std::endl;
		mes->setServiced();
		
		
		//-- If the ID of the reply is < -, the cache line is for the MCPU and not for the VAS Tile
		if(mes->getCoreId() != (uint16_t)-1) {
			out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile()), 0);
		} else {
			std::cout << "For the MCPU" << std::endl;
		}
	}


	void MemoryCPUWrapper::notifyCompletion_() {
	}

	void MemoryCPUWrapper::setRequestManager(std::shared_ptr<EventManager> r) {
		request_manager_ = r;
	}
}
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab:
