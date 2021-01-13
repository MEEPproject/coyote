#ifndef __COMMAND_SCHEDULER_HH__
#define __COMMAND_SCHEDULER_HH__

#include <memory>
#include <tuple>
#include "Request.hpp"

namespace spike_model
{
    class CommandSchedulerIF
    {
        public: 

            virtual void addCommand(std::shared_ptr<BankCommand> b)=0;
            
            virtual std::shared_ptr<BankCommand> getNextCommand()=0;

            virtual bool hasCommands()=0;
    };
}
#endif
