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
        /*!
         * \class spike_model::FifoCommandScheduler
         * \brief A simple FIFO BankCommand scheduler
         */
        public: 
            /*!
             * \brief Constructor for FifoCommandScheduler
             */
            FifoCommandScheduler();

            /*!
            * \brief Add a command to the scheduler
            * \param b The bank command to add
            */
            void addCommand(std::shared_ptr<BankCommand> b) override;
            
            /*!

            * \brief Get the next command that has to be issued, which will be the 
            * FIFO queue
            * \return The command
            */
            std::shared_ptr<BankCommand> getNextCommand() override;

            /*!
            * \brief Check if the scheduler has more commands
            * \return True if the scheduler has more commands
            */
            bool hasCommands() override;
            
        private:
            std::queue<std::shared_ptr<BankCommand>> commands;

    };  
}
#endif
