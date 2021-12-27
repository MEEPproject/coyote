#include "OldestReadyCommandScheduler.hpp"

namespace spike_model
{
    OldestReadyCommandScheduler::OldestReadyCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : CommandSchedulerIF(latencies, num_banks), commands(){}

    void OldestReadyCommandScheduler::addCommand(std::shared_ptr<BankCommand> b)
    {
        commands.push_back(b);
    }

    std::shared_ptr<BankCommand> OldestReadyCommandScheduler::selectCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res=nullptr;

        std::list<std::shared_ptr<BankCommand>>::iterator it = commands.begin();
        while (it != commands.end() && res==nullptr)
        {
            if(checkTiming(*it, currentTimestamp))
            {
                res=*it;
                commands.erase(it);
                printf("Returning command for bank %lu\n", res->getDestinationBank());
                printf("The list now is:\n\t[");
                std::list<std::shared_ptr<BankCommand>>::iterator it2 = commands.begin();
                while(it2 != commands.end())
                {
                    printf("%lu, ", (*it2)->getDestinationBank());
                    it2++;
                }
                printf("]\n");
            }
            else
            {
                it++;
            }
        }
        return res;
    }

    bool OldestReadyCommandScheduler::hasCommands()
    {
        return commands.size()>0;
    }
}
