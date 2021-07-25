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
			latency_(p->latency) {
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
		//DO sth (Reggy)
		mem_req_pipeline.push(mes);
		if(bypass_incoming_idle) {
			controller_cycle_event_s.schedule();
			bypass_incoming_idle = false;
		}
		
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
		
		if(mcpu_incoming_idle) {
			controller_cycle_event_v.schedule();
			mcpu_incoming_idle = false;
		}
	}


	void MemoryCPUWrapper::controllerCycle_vec() {
		schedule_incoming_mem_ops();
		//schedule_mem_ops_to_mc();
		//schedule_outgoing_mem_ops();
		
		if(sched_incoming.size() == 0) {
			mcpu_incoming_idle = true;
		} else {
			controller_cycle_event_v.schedule(1);
		}
	}
	// Do sth (Reggy)
	void MemoryCPUWrapper::controllerCycle_sca() {
        schedule_incoming_mem_ops();
		
		if(mem_req_pipeline.size() == 0) {
			bypass_incoming_idle = true;
		} else {
			controller_cycle_event_s.schedule(1);
		}

	}
	
	void MemoryCPUWrapper::schedule_incoming_mem_ops() {
		if(sched_incoming.size() == 0) {
			return;
		}
		
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
	
	
	void MemoryCPUWrapper::schedule_mem_ops_to_mc() {
		if(mem_req_pipeline.size() == 0) {
			return;
		}
		
		//-- Get the oldest Cache Request from the queue
		std::shared_ptr<CacheRequest> instr_for_mc = mem_req_pipeline.front();
		
		//-- Send to the MC
		out_port_mc_.send(instr_for_mc, 0);
	}
	
	
	void MemoryCPUWrapper::schedule_outgoing_mem_ops() {
		
	}
	
	
	
	void MemoryCPUWrapper::memOp_unit(std::shared_ptr<MCPUInstruction> instr) {
				
		//-- get the number of elements loaded by 1 memory request
		uint number_of_elements_per_request = line_size_ / (uint32_t)instr->get_width();
		uint number_of_mem_ops = vvl_ / number_of_elements_per_request;
		
		if(number_of_mem_ops == 0) number_of_mem_ops=1;
		
		for(uint i=0; i<number_of_mem_ops; ++i) {
			//-- Generate a cache line request
			std::shared_ptr<CacheRequest> memory_request = std::make_shared<CacheRequest>(
						instr->get_baseAddress()+i*64, 
						(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
									CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
						0,
						getClock()->currentCycle(), 
						(uint16_t)-1);
			
			//-- schedule this request for the MC
			mem_req_pipeline.push(memory_request);
			//mem_req_pipeline.push(mes);
			//DO sth (Reggy)
		if(mcpu_incoming_idle) {
			controller_cycle_event_v.schedule();
			mcpu_incoming_idle = false;
		}
			
		}
	}
	
	void MemoryCPUWrapper::memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr) {
		
		std::vector<uint64_t> indices = instr->get_index();
		for(std::vector<uint64_t>::iterator it = indices.begin(); it != indices.end(); ++it) {
			//-- TODO Generate cache line requests
			//std::shared_ptr<MCPUInstruction> mem_op = std::make_shared<MCPUInstruction>(*instr);
			//
			//mem_op->set_baseAddress(instr->get_baseAddress() + *it);
			//mem_req_pipeline.push(mem_op);
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