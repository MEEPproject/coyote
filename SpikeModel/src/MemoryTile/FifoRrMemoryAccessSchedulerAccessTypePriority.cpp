#include "FifoRrMemoryAccessSchedulerAccessTypePriority.hpp"

namespace spike_model
{
    FifoRrMemoryAccessSchedulerAccessTypePriority::FifoRrMemoryAccessSchedulerAccessTypePriority(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate) : MemoryAccessSchedulerIF(b, num_banks, write_allocate), fetch_queues(num_banks), load_queues(num_banks), store_queues(num_banks){}

    void FifoRrMemoryAccessSchedulerAccessTypePriority::putRequest(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        switch(req->getType())
        {
            case CacheRequest::AccessType::FETCH:
                fetch_queues[bank].push(req);
                break;

            case CacheRequest::AccessType::LOAD:
                load_queues[bank].push(req);
                break;

            case CacheRequest::AccessType::STORE:
                store_queues[bank].push(req);
                break;

            case CacheRequest::AccessType::WRITEBACK:
                store_queues[bank].push(req);
                break;
        }

        //If there is only one access total in the queues for the bank, mark the bank as scheduleable
        if(fetch_queues[bank].size()+load_queues[bank].size()+store_queues[bank].size()==1 && !pending_command[bank])
        {
            banks_to_schedule.push(bank);
        }
    }

    std::shared_ptr<CacheRequest> FifoRrMemoryAccessSchedulerAccessTypePriority::getRequest(uint64_t bank)
    {
        std::shared_ptr<CacheRequest> res;
        if(fetch_queues[bank].size()>0)
        {
            res=fetch_queues[bank].front();
        }
        else if(load_queues[bank].size()>0)
        {
            res=load_queues[bank].front();
        }
        else if(store_queues[bank].size()>0)
        {
            res=store_queues[bank].front();
        }
        return res;
    }
            
    uint64_t FifoRrMemoryAccessSchedulerAccessTypePriority::getNextBank()
    {
        uint64_t res=banks_to_schedule.front();
        banks_to_schedule.pop();
        return res;
    }
    
    bool FifoRrMemoryAccessSchedulerAccessTypePriority::hasBanksToSchedule()
    {
        return banks_to_schedule.size()>0;
    }
    
    void FifoRrMemoryAccessSchedulerAccessTypePriority::notifyRequestCompletion(std::shared_ptr<CacheRequest> req)
    {
        uint64_t bank=req->getMemoryBank();
        switch(req->getType())
        {
            case CacheRequest::AccessType::FETCH:
                fetch_queues[bank].pop();
                break;

            case CacheRequest::AccessType::LOAD:
                load_queues[bank].pop();
                break;

            case CacheRequest::AccessType::STORE:
                store_queues[bank].pop();
                break;

            case CacheRequest::AccessType::WRITEBACK:
                store_queues[bank].pop();
                break;
        }
    }
    
    uint64_t FifoRrMemoryAccessSchedulerAccessTypePriority::getQueueOccupancy()
    {
       uint64_t res=0;
       for(auto &queue : fetch_queues)
       {
            res=res+queue.size();
       }
       for(auto &queue : load_queues)
       {
            res=res+queue.size();
       }
       for(auto &queue : store_queues)
       {
            res=res+queue.size();
       }
       return res;
    }
    
    void FifoRrMemoryAccessSchedulerAccessTypePriority::rescheduleBank(uint64_t bank)
    {
        if(fetch_queues[bank].size()+load_queues[bank].size()+store_queues[bank].size()>0)
        {
            banks_to_schedule.push(bank);
        }

    }
}
