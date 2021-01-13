#include "FifoRrMemoryAccessScheduler.hpp"

namespace spike_model
{
    FifoRrMemoryAccessScheduler::FifoRrMemoryAccessScheduler(uint64_t num_banks) : request_queues(num_banks){}

    void FifoRrMemoryAccessScheduler::putRequest(std::shared_ptr<Request> req, uint64_t bank)
    {
        request_queues[bank].push(req);
        if(request_queues[bank].size()==1)
        {
            banks_to_schedule.push(bank);
        }
    }

    std::shared_ptr<Request> FifoRrMemoryAccessScheduler::getRequest(uint64_t bank)
    {
        std::shared_ptr<Request> res=request_queues[bank].front();
        return res;
    }
            
    uint64_t FifoRrMemoryAccessScheduler::getNextBank()
    {
        uint64_t res=banks_to_schedule.front();
        banks_to_schedule.pop();
        return res;
    }
    
    bool FifoRrMemoryAccessScheduler::hasIdleBanks()
    {
        return banks_to_schedule.size()>0;
    }
    
    void FifoRrMemoryAccessScheduler::notifyRequestCompletion(uint64_t bank)
    {
        request_queues[bank].pop();
        if(request_queues[bank].size()>0)
        {
            banks_to_schedule.push(bank);
        }
    }
}
