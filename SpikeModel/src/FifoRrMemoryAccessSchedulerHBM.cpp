#include "FifoRrMemoryAccessScheduler_HBM.hpp"

namespace spike_model
{
    FifoRrMemoryAccessSchedulerHBM::FifoRrMemoryAccessSchedulerHBM(uint64_t num_banks) : request_queues(num_banks){}

    void FifoRrMemoryAccessSchedulerHBM::putRequest(std::shared_ptr<Request> req, uint64_t bank)
    {
        request_queues[bank].push(req);
        if(request_queues[bank].size()==1)
        {
            banks_to_schedule.push(bank);
        }
    }

    std::shared_ptr<Request> FifoRrMemoryAccessSchedulerHBM::getRequest(uint64_t bank)
    {
        std::shared_ptr<Request> res=request_queues[bank].front();
        return res;
    }

    uint64_t FifoRrMemoryAccessSchedulerHBM::getNextBank()
    {
        uint64_t res=banks_to_schedule.front();
        banks_to_schedule.pop();
        return res;
    }

    bool FifoRrMemoryAccessSchedulerHBM::hasIdleBanks()
    {
        return banks_to_schedule.size()>0;
    }

    void FifoRrMemoryAccessSchedulerHBM::notifyRequestCompletion(uint64_t bank)
    {
        request_queues[bank].pop();
        if(request_queues[bank].size()>0)
        {
            banks_to_schedule.push(bank);
        }
    }
}
