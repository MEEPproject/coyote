#ifndef __COMMAND_SCHEDULER_HH__
#define __COMMAND_SCHEDULER_HH__

#include <memory>
#include <tuple>
#include "Request.hpp"

namespace spike_model
{
    class CommandSchedulerIF
    {
        /*!
         * \class spike_model::CommandSchedulerIF
         * \brief Abstract class representing the minimum operation of a command scheduler for BankCommand instances.
         *
         */
        public: 

            /*!
            * \brief Add a command to the scheduler
            * \param b b The bank command to add
            */
            virtual void addCommand(std::shared_ptr<BankCommand> b)=0;
            
            /*!
            * \brief Get the next command that has to be issued
            * \return The command
            */
            virtual std::shared_ptr<BankCommand> getNextCommand()=0;

            /*!
            * \brief Check if the scheduler has more commands
            * \return True if the scheduler has more commands
            */
            virtual bool hasCommands()=0;
    };
}
#endif
