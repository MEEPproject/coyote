#include "FifoRrMemoryAccessScheduler.hpp"

namespace spike_model
{
    FifoRrMemoryAccessScheduler::FifoRrMemoryAccessScheduler(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate) : MemoryAccessSchedulerIF(b, num_banks, write_allocate), request_queues(num_banks){}

    void FifoRrMemoryAccessScheduler::putRequest(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        request_queues[bank].push(req);
        if(request_queues[bank].size()==1)
        {
            banks_to_schedule.push(bank);
        }
    }

    std::shared_ptr<CacheRequest> FifoRrMemoryAccessScheduler::getRequest(uint64_t bank)
    {
        std::shared_ptr<CacheRequest> res=request_queues[bank].front();
        return res;
    }
            
    uint64_t FifoRrMemoryAccessScheduler::getNextBank()
    {
        uint64_t res=banks_to_schedule.front();
        banks_to_schedule.pop();
        return res;
    }
    
    bool FifoRrMemoryAccessScheduler::hasBanksToSchedule()
    {
        return banks_to_schedule.size()>0;
    }
    
    void FifoRrMemoryAccessScheduler::notifyRequestCompletion(std::shared_ptr<CacheRequest> req)
    {
        uint64_t bank=req->getMemoryBank();
        request_queues[bank].pop();
    }
            
    uint64_t FifoRrMemoryAccessScheduler::getQueueOccupancy()
    {
        uint64_t res=0;
       for(auto &queue : request_queues)
       {
            res=res+queue.size();
       }
       return res;
    }
    
    std::shared_ptr<CacheRequest> FifoRrMemoryAccessScheduler::notifyCommandCompletion(std::shared_ptr<BankCommand> c)
    {
        std::shared_ptr<CacheRequest> serviced_request=MemoryAccessSchedulerIF::notifyCommandCompletion(c);
        if(request_queues[c->getDestinationBank()].size()>0)
        {
            banks_to_schedule.push(c->getDestinationBank());
        }
        return serviced_request;
    }
}
