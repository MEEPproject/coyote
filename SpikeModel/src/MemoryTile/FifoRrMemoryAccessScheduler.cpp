#include "FifoRrMemoryAccessScheduler.hpp"

namespace spike_model
{
    FifoRrMemoryAccessScheduler::FifoRrMemoryAccessScheduler(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate) : MemoryAccessSchedulerIF(b, num_banks, write_allocate), request_queues(num_banks){}

    void FifoRrMemoryAccessScheduler::putRequest(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        request_queues[bank].push(req);
        std::cout << "[Push] Bank " << bank << " has " << request_queues[bank].size() << " requests\n";
        if(request_queues[bank].size()==1 && !pending_command[bank])
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
        std::cout << "[Pop] Bank " << bank << " has " << request_queues[bank].size() << " requests\n";
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
             
    void FifoRrMemoryAccessScheduler::rescheduleBank(uint64_t bank)
    {
        if(request_queues[bank].size()>0)
        {
            std::cout << "Rescheduling bank " << bank << "\n";
            banks_to_schedule.push(bank);
        }
        else
        {
            std::cout << "Not rescheduling bank (no requests)" << bank << "\n";
        }
    }
}
