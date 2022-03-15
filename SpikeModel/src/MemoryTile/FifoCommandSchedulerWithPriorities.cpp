#include "FifoCommandSchedulerWithPriorities.hpp"
#include "CacheRequest.hpp"

namespace spike_model
{
    FifoCommandSchedulerWithPriorities::FifoCommandSchedulerWithPriorities(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : FifoCommandScheduler(latencies, num_banks){}

    void FifoCommandSchedulerWithPriorities::addCommand(std::shared_ptr<BankCommand> b)
    {
        if(b->isHighPriority())
        {
            printf("Adding priority command for address %#lx\n", b->getRequest()->getAddress());
            priority_commands.push(b);
        }
        else
        {
            printf("Adding regular command for address %#lx\n", b->getRequest()->getAddress());
            FifoCommandScheduler::addCommand(b);
        }
    }

    std::shared_ptr<BankCommand> FifoCommandSchedulerWithPriorities::selectCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res;
        if(priority_commands.size()>0)
        {
            res=priority_commands.front();
            if(!checkTiming(res, currentTimestamp))
            {
                if(FifoCommandScheduler::hasCommands())
                {
                    printf("Priority command is not ready!!\n");
                    res=FifoCommandScheduler::selectCommand(currentTimestamp);
                }
                else
                {
                    res=nullptr;
                }
            }
            else
            {
                printf("Prioriuty command issued\n");
                priority_commands.pop();
            }
        }
        else
        {
            res=FifoCommandScheduler::selectCommand(currentTimestamp);
        }
        return res;
    }

    bool FifoCommandSchedulerWithPriorities::hasCommands()
    {
        return priority_commands.size()>0 || FifoCommandScheduler::hasCommands();
    }
}
