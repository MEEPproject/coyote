#include "FifoCommandScheduler.hpp"

namespace spike_model
{
    FifoCommandScheduler::FifoCommandScheduler() : commands(){}

    void FifoCommandScheduler::addCommand(std::shared_ptr<BankCommand> b)
    {
        commands.push(b);
    }

    std::shared_ptr<BankCommand> FifoCommandScheduler::getNextCommand()
    {
        std::shared_ptr<BankCommand> res=commands.front();
        commands.pop();
        return res;
    }

    bool FifoCommandScheduler::hasCommands()
    {
        return commands.size()>0;
    }
}
