#ifndef __COMMAND_SCHEDULER_HH__
#define __COMMAND_SCHEDULER_HH__

#include <memory>
#include <vector>

#include "BankCommand.hpp"

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
             * \brief Constructor for CommandSchedulerIF
             * \param latencies The DRAM memspec
             */
            CommandSchedulerIF(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks);

            /*!
            * \brief Add a command to the scheduler
            * \param b The bank command to add
            */
            virtual void addCommand(std::shared_ptr<BankCommand> b)=0;
            
            /*!
            * \brief Get the next command that has to be issued
            * \return The command
            */
            std::shared_ptr<BankCommand> getNextCommand(uint64_t currentTimestamp);
            
            /*!
            * \brief Check if the scheduler has more commands
            * \return True if the scheduler has more commands
            */
            virtual bool hasCommands()=0;

        protected:
            bool checkTiming(std::shared_ptr<BankCommand> c, uint64_t currentTimestamp);
            
        private:
            std::shared_ptr<std::vector<uint64_t>> latencies;
            std::vector<uint64_t> last_timestamp_per_command_type;
            std::vector<uint64_t> last_read_timestamp_per_bank;
            std::vector<uint64_t> last_activate_timestamp_per_bank;
            std::vector<bool> access_after_activate_per_bank;
            std::vector<uint64_t> last_precharge_timestamp_per_bank;

            /*!
            * \brief Pick the next command to submit to a bank at the current timestamp
            * \param currentTimestamp The timestamp for the scheduling
            * \return The next command to schedule (nullptr if no command is available).
            */
            virtual std::shared_ptr<BankCommand> selectCommand(uint64_t currentTimestamp)=0;
    };
}
#endif
