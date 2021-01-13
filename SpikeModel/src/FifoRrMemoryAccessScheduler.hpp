#ifndef __FIFO_MEMORY_ACCESS_SCHEDULER_HH__
#define __FIFO_MEMORY_ACCESS_SCHEDULER_HH__

#include <queue>
#include <vector>
#include "Request.hpp"
#include "MemoryAccessSchedulerIF.hpp"

namespace spike_model
{
    class FifoRrMemoryAccessScheduler : public MemoryAccessSchedulerIF
    {
        public:
 
            FifoRrMemoryAccessScheduler(uint64_t num_banks);

            void putRequest(std::shared_ptr<Request> req, uint64_t bank) override;
            
            std::shared_ptr<Request> getRequest(uint64_t bank) override;
            uint64_t getNextBank() override;
            
            virtual bool hasIdleBanks() override;
            
            void notifyRequestCompletion(uint64_t bank) override;
            
        private:
            std::vector<std::queue<std::shared_ptr<Request>>> request_queues;
            std::queue<uint64_t> banks_to_schedule; //A bank id will be in this queue if the associated bank queue has elements and a request to the buffer is not currently in process
    };
}
#endif
