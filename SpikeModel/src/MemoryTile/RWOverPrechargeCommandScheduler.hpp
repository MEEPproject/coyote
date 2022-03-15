#ifndef __RW_OVER_PRECHARGE_COMMAND_SCHEDULER_HH__
#define __RW_OVER_PRECHARGE_COMMAND_SCHEDULER_HH__

#include <memory>
#include <queue>
#include <set>
#include "BankCommand.hpp"
#include "CommandSchedulerIF.hpp"

namespace spike_model
{
    class RWOverPrechargeCommandScheduler : public CommandSchedulerIF
    {
        /*!
         * \class spike_model::FifoCommandScheduler
         * \brief A simple FIFO BankCommand scheduler
         */
        public: 
            /*!
             * \brief Constructor for FifoCommandScheduler
             * \param latencies The DRAM memspec
             */
            RWOverPrechargeCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks);

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
            std::queue<std::shared_ptr<BankCommand>> reads_and_writes;
            std::queue<std::shared_ptr<BankCommand>> precharges_and_activates;

            /*!
            * \brief Pick the next command to submit to a bank at the current timestamp. The command will be picked FIFO (if timing requirements are met)
            * \param currentTimestamp The timestamp for the scheduling
            * \return The next command to schedule (nullptr if no command is available).
            */
            std::shared_ptr<BankCommand> selectCommand(uint64_t currentTimestamp) override;
    };  
}
#endif
