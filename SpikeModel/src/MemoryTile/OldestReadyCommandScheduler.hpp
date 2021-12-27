#ifndef __OLDEST_READY_COMMAND_SCHEDULER_HH__
#define __OLDEST_READY_COMMAND_SCHEDULER_HH__

#include <memory>
#include <list>
#include "BankCommand.hpp"
#include "CommandSchedulerIF.hpp"

namespace spike_model
{
    class OldestReadyCommandScheduler : public CommandSchedulerIF
    {
        /*!
         * \class spike_model::OldestReadyCommandScheduler
         * \brief A simple FIFO BankCommand scheduler
         */
        public: 
            /*!
             * \brief Constructor for OldestReadyCommandScheduler
             * \param latencies The DRAM memspec
             */
            OldestReadyCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks);

            /*!
            * \brief Add a command to the scheduler
            * \param b The bank command to add
            */
            void addCommand(std::shared_ptr<BankCommand> b) override;
            

            /*!
            * \brief Check if the scheduler has more commands
            * \return True if the scheduler has more commands
            */
            bool hasCommands() override;
            
        private:
            std::list<std::shared_ptr<BankCommand>> commands;

            /*!
            * \brief Pick the next command to submit to a bank at the current timestamp. The command will be the oldest that meets the timing requirements
            * \param currentTimestamp The timestamp for the scheduling
            * \return The next command to schedule (nullptr if no command is available).
            */
            std::shared_ptr<BankCommand> selectCommand(uint64_t currentTimestamp) override;
    };  
}
#endif
