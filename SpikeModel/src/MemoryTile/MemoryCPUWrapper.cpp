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

	//-- A transaction for the Cache
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
		out_port_mc_.send(mes, 0);
	}

	//-- A memory transaction to be handled by the MCPU
        //Note that VVL of 0 is received when the simulation is setting up
        //and the processor objects are created.
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUSetVVL> mes) {
	    std::cout <<  "  AVL " << mes->getAVL() << " received from MCPU: Core "
                      << mes->getCoreId() <<std::endl;
            mcpu_req.push_back(mes);
            issue_mcpu_event_.schedule(1);
	}

	//-- An instruction for the MCPU
    void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUInstruction> r) {
        std::cout << "MCPU: Memory instruction received. Core: " << r->getCoreId() << ", BaseAddress: " << r->getAddress() << ", Size: " << (int)r->get_width() << ", sub op? " << (int)r->get_suboperation() << std::endl;
        switch(r->get_operation()) {
        	case MCPUInstruction::Operation::LOAD:
        		std::cout << "\tIt is a load" << std::endl;
        		break;
        	case MCPUInstruction::Operation::STORE:
        		std::cout << "\tIt is a store" << std::endl;
        		break;
        	default:
        		std::cout << "MCPU: I do not know, what the core wants from me!" << std::endl;
        }
        //TODO: Something interesting :)
        //Generate memory accesses/ scratchpad requests
    }




	/////////////////////////////////////
	//-- Command Execution
	/////////////////////////////////////
	void MemoryCPUWrapper::issueMCPU_() {
            std::shared_ptr<MCPUSetVVL> mes = mcpu_req.front();
	    mes->setVVL(mes->getAVL());
	    std::cout << "MCPU: Returning VVL " << mes->getVVL() << " to core " << mes->getCoreId() << std::endl;
	    mes->setServiced();
            std::cout << "MCPU getclock " << getClock()->currentCycle() << std::endl;
	    out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, line_size_, mes->getMemoryCPU(), mes->getSourceTile()), 0);

            //-- Are there any messages left in the queue?
            mcpu_req.pop_front();
	    if(mcpu_req.size() > 0) {
	        issue_mcpu_event_.schedule(1);
	    }
	}





	/////////////////////////////////////
	//-- Message handling from the MC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_mc_(const std::shared_ptr<CacheRequest> &mes)	{
		count_requests_mc_++;
		//std::cout << "Returning: " << mes << ", coreID: " << mes->getCoreId() << std::endl;
		mes->setServiced();
                //std::cout << "MCPU getclock " << getClock()->currentCycle() << std::endl;
		out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getMemoryController(), mes->getHomeTile()), 0);
	}


	void MemoryCPUWrapper::notifyCompletion_() {
	}

	void MemoryCPUWrapper::setRequestManager(std::shared_ptr<EventManager> r) {
		request_manager_ = r;
	}
}
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab:
