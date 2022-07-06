#include "FifoCommandScheduler.hpp"

namespace coyote
{
    FifoCommandScheduler::FifoCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : CommandSchedulerIF(latencies, num_banks), commands(){}

    void FifoCommandScheduler::addCommand(std::shared_ptr<BankCommand> b)
    {
        commands.push(b);
    }

    std::shared_ptr<BankCommand> FifoCommandScheduler::selectCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res=commands.front();
        if(checkTiming(res, currentTimestamp))
        {
            commands.pop();
        }
        else
        {
            res=nullptr;
        }
        return res;
    }

    bool FifoCommandScheduler::hasCommands()
    {
        return commands.size()>0;
    }
}
