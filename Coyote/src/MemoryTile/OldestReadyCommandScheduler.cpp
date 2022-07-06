#include "OldestReadyCommandScheduler.hpp"

namespace coyote
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
