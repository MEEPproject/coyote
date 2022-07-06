#ifndef __FIFO_COMMAND_SCHEDULER_HH__
#define __FIFO_COMMAND_SCHEDULER_HH__

#include <memory>
#include <queue>
#include <set>
#include "BankCommand.hpp"
#include "CommandSchedulerIF.hpp"

namespace coyote
{
    class FifoCommandScheduler : public CommandSchedulerIF
    {
        /*!
         * \class coyote::FifoCommandScheduler
         * \brief A simple FIFO BankCommand scheduler
         */
        public: 
            /*!
             * \brief Constructor for FifoCommandScheduler
             * \param latencies The DRAM memspec
             */
            FifoCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks);

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

        protected:
            /*!
            * \brief Pick the next command to submit to a bank at the current timestamp. The command will be picked FIFO (if timing requirements are met)
            * \param currentTimestamp The timestamp for the scheduling
            * \return The next command to schedule (nullptr if no command is available).
            */
            std::shared_ptr<BankCommand> selectCommand(uint64_t currentTimestamp) override;
            
        private:
            std::queue<std::shared_ptr<BankCommand>> commands;

    };  
}
#endif
