// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 

#ifndef __COMMAND_SCHEDULER_HH__
#define __COMMAND_SCHEDULER_HH__

#include <memory>
#include <vector>

#include "BankCommand.hpp"

namespace coyote
{
    class CommandSchedulerIF
    {
        /*!
         * \class coyote::CommandSchedulerIF
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
            * \brief Set the burts length of the banks handled by this scheduler
            * \param l The burst length
            */
            void setBurstLength(uint8_t l);

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
            std::vector<uint64_t> last_write_timestamp_per_bank;
            std::vector<uint64_t> last_activate_timestamp_per_bank;
            std::vector<bool> access_after_activate_per_bank;
            std::vector<uint64_t> last_precharge_timestamp_per_bank;

            uint8_t burst_length;

            /*!
            * \brief Pick the next command to submit to a bank at the current timestamp
            * \param currentTimestamp The timestamp for the scheduling
            * \return The next command to schedule (nullptr if no command is available).
            */
            virtual std::shared_ptr<BankCommand> selectCommand(uint64_t currentTimestamp)=0;
    };
}
#endif
