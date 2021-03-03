#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryCPU.hpp"

namespace spike_model {
	const char MemoryCPU::name[] = "memory_cpu";

	////////////////////////////////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////////////////////////////////

	MemoryCPU::MemoryCPU(sparta::TreeNode *node, const MemoryCPUParameterSet *p):
		sparta::Unit(node),
		latency_(p->latency) {
		in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPU, receiveMessage_noc_, std::shared_ptr<NoCMessage>));
		in_port_mc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPU, receiveMessage_mc_, std::shared_ptr<Request>));
	}

	void MemoryCPU::receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes) {
		count_requests_noc_++;
		std::shared_ptr<Request> m = mes->getRequest();

		switch(m->getType()) {
			case Request::AccessType::FETCH:
			case Request::AccessType::LOAD:
				count_load_++;
				std::cout << m << "Loading: " << m << ", coreID: " << m->getCoreId() << std::endl;

				break;
			case Request::AccessType::WRITEBACK:
			case Request::AccessType::STORE:	count_store_++; break;
		}
		out_port_mc_.send(m, 0);
	}

	void MemoryCPU::receiveMessage_mc_(const std::shared_ptr<Request> &mes)	{
		count_requests_mc_++;
		out_port_noc_.send(request_manager_->getMemoryReplyMessage(mes), 0);
	}

	void MemoryCPU::controllerCycle_() {
	}

	void MemoryCPU::notifyCompletion_() {
	}

	void MemoryCPU::setRequestManager(std::shared_ptr<RequestManagerIF> r) {
		request_manager_=r;
	}
}
