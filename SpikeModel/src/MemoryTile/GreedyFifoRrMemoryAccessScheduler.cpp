#include "GreedyFifoRrMemoryAccessScheduler.hpp"

namespace spike_model
{
    GreedyFifoRrMemoryAccessScheduler::GreedyFifoRrMemoryAccessScheduler(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate) : FifoRrMemoryAccessScheduler(b, num_banks, write_allocate){}

             
    void GreedyFifoRrMemoryAccessScheduler::rescheduleBank(uint64_t bank)
    {
        if(request_queues[bank].size()>0)
        {
            printf("The size is %lu\n", request_queues[bank].size());
            std::shared_ptr<CacheRequest> req=getRequest(bank);

            uint64_t row_to_schedule=req->getRow();

            printf("The open row is %lu and the one to schedule is %lu\n", (*banks)[bank]->getOpenRow(), row_to_schedule);

            //if((*banks)[bank]->isOpen() && req->getSizeRequestedToMemory()<req->getSize() && (*banks)[bank]->getOpenRow()==row_to_schedule) 
            if(!(*banks)[bank]->isOpen() || (*banks)[bank]->getOpenRow()!=row_to_schedule) 
            {
                printf("Greedily adding bank %lu!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", bank);
                banks_to_schedule.push_front(bank);
                //banks_to_schedule.push_back(bank);
            }
            else
            {
                banks_to_schedule.push_back(bank);
            }
        }
    }
    
    std::shared_ptr<BankCommand> GreedyFifoRrMemoryAccessScheduler::getCommand(uint64_t bank)
    {
        std::shared_ptr<BankCommand> res=MemoryAccessSchedulerIF::getCommand(bank);
        //if(res!=nullptr && res->getRequest()->getSizeRequestedToMemory()>0)
        if(res!=nullptr && (res->getType()==BankCommand::CommandType::ACTIVATE || res->getType()==BankCommand::CommandType::PRECHARGE))
        {
            res->setHighPriority();
        }
        return res;
    }
}
