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
