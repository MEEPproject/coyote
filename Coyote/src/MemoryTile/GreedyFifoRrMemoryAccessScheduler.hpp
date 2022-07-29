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

#ifndef __GREEDY_FIFO_MEMORY_ACCESS_SCHEDULER_HH__
#define __GREEDY_FIFO_MEMORY_ACCESS_SCHEDULER_HH__

#include <queue>
#include <vector>
#include "CacheRequest.hpp"
#include "BankCommand.hpp"
#include "FifoRrMemoryAccessScheduler.hpp"

namespace coyote
{
    class GreedyFifoRrMemoryAccessScheduler : public FifoRrMemoryAccessScheduler
    {
        /*!
         * \class coyote::FifoRrMemoryAccessScheduler
         * \brief A simple memory access scheduler that follows the following policy.
         * 1. The next bank to schedule is chosen in round-robin
         * 2. The next request to schedule is chosen in FIFO
         */
        public:
 
            /*!
            * \brief Constructor for FifoRrMemoryAccessScheduler
            * \param b The banks handled by the scheduler
            * \param num_banks The number of banks
            * \param write_allocate True if writes need to allocate in cache
            */
            GreedyFifoRrMemoryAccessScheduler(std::shared_ptr<std::vector<MemoryBank *>> banks, uint64_t num_banks, bool write_allocate);
    
            std::shared_ptr<BankCommand> getCommand(uint64_t bank) override;

       protected: 
            
            /*!
             * \brief Mark a bank as ready for scheduling if it has any pending requests. If the request will generate read/write next, 
             * it will be greedily pushed to the front of the scheduling queue
             * \param bank The bank to reschedule
             */
            void rescheduleBank(uint64_t bank) override;
    };
}
#endif
