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

	//-- A request to the MC
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::CacheRequest> mes) {
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

	//-- a request for the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPURequest> mes) {
			std::cout << "Requesting vec len from MCPU from core " << mes->getCoreId()  << " and vector len " << mes->getRequestedVecLen() << std::endl;
			mcpu_req.push_back(mes);
			issue_mcpu_event_.schedule(1);
	}

	void MemoryCPUWrapper::issueMCPU_() {
		std::shared_ptr<MCPURequest> mes = mcpu_req.front();
		mes->setReturnedVecLen(mes->getRequestedVecLen());
		std::cout << "Returning vec len from MCPU from core " << mes->getCoreId() << " and vector len " << mes->getReturnedVecLen() << std::endl;
		mes->setServiced();
		out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, line_size_, mes->getSourceTile()), 0);

		mcpu_req.pop_front();
		if(mcpu_req.size() > 0) {
			issue_mcpu_event_.schedule(1);
		}
	}
            

    void MemoryCPUWrapper::handle(std::shared_ptr<spike_model::MCPUInstruction> r)
    {
        printf("Memory instruction received in the MCPU\n");
        //TODO: Something interesting :)
        //Generate memory accesses/ scratchpad requests
    }


	/////////////////////////////////////
	//-- Message handling from the MC
	/////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_mc_(const std::shared_ptr<CacheRequest> &mes)	{
		count_requests_mc_++;
		//std::cout << "Returning: " << mes << ", coreID: " << mes->getCoreId() << std::endl;
		mes->setServiced();
		out_port_noc_.send(std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size_, mes->getHomeTile()), 0);
	}


	void MemoryCPUWrapper::notifyCompletion_() {
	}

	void MemoryCPUWrapper::setRequestManager(std::shared_ptr<EventManager> r) {
		request_manager_ = r;
	}
}
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab: