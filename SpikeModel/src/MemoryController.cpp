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
    latency_(p->latency),
    line_size_(p->line_size)
    {
        in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryController, issueAck_, std::shared_ptr<NoCMessage>));
    }
            void logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);
            
            void logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);
            
            void logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);
            
            void logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);

            void logMissServiced(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);
        
    void MemoryController::issueAck_(const std::shared_ptr<NoCMessage> & mes)
    {
        //std::cout << "Issuing ack from memory controller to request from core " << mes->getRequest()->getCoreId() << " for address " << mes->getRequest()->getAddress() << "\n";
        if(trace_)
        {
            logger_.logMemoryControllerOperation(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getRequest()->getPC(), mes->getRequest()->getAddress());
        }
        out_port_noc_.send(std::make_shared<NoCMessage>(mes->getRequest(), NoCMessageType::MEMORY_ACK, line_size_), latency_);
        count_requests_++;
    }
}
