#ifndef __FIFO_COMMAND_SCHEDULER_HH__
#define __FIFO_COMMAND_SCHEDULER_HH__

#include <memory>
#include <queue>
#include <set>
#include "BankCommand.hpp"
#include "CommandSchedulerIF.hpp"

namespace spike_model
{
    class FifoCommandScheduler : public CommandSchedulerIF
    {
        public: 
            FifoCommandScheduler();

            void addCommand(std::shared_ptr<BankCommand> b) override;
            
            std::shared_ptr<BankCommand> getNextCommand() override;

            bool hasCommands() override;

        private:
            std::queue<std::shared_ptr<BankCommand>> commands;

    };  
}
#endif
