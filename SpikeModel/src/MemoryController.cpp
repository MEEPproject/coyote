#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryController.hpp"
#include <chrono>

namespace spike_model
{
    const char MemoryController::name[] = "memory_controller";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    MemoryController::MemoryController(sparta::TreeNode *node, const MemoryControllerParameterSet *p):
    sparta::Unit(node),
    latency_(p->latency)
    {
        in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryController, issueAck_, std::shared_ptr<NoCMessage>));
    }
        
    void MemoryController::issueAck_(const std::shared_ptr<NoCMessage> & mes)
    {
        //std::cout << "Issuing ack from memory controller to request from core " << mes->getRequest()->getCoreId() << " for address " << mes->getRequest()->getAddress() << "\n";
        out_port_noc_.send(std::make_shared<NoCMessage>(mes->getRequest(), NoCMessageType::MEMORY_ACK), latency_);
        count_requests_++;
    }
}
