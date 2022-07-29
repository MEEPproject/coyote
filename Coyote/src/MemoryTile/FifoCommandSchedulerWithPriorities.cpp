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

#include "FifoCommandSchedulerWithPriorities.hpp"
#include "CacheRequest.hpp"

namespace coyote
{
    FifoCommandSchedulerWithPriorities::FifoCommandSchedulerWithPriorities(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : FifoCommandScheduler(latencies, num_banks){}

    void FifoCommandSchedulerWithPriorities::addCommand(std::shared_ptr<BankCommand> b)
    {
        if(b->isHighPriority())
        {
            printf("Adding priority command for address %#lx\n", b->getRequest()->getAddress());
            priority_commands.push(b);
        }
        else
        {
            printf("Adding regular command for address %#lx\n", b->getRequest()->getAddress());
            FifoCommandScheduler::addCommand(b);
        }
    }

    std::shared_ptr<BankCommand> FifoCommandSchedulerWithPriorities::selectCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res;
        if(priority_commands.size()>0)
        {
            res=priority_commands.front();
            if(!checkTiming(res, currentTimestamp))
            {
                if(FifoCommandScheduler::hasCommands())
                {
                    printf("Priority command is not ready!!\n");
                    res=FifoCommandScheduler::selectCommand(currentTimestamp);
                }
                else
                {
                    res=nullptr;
                }
            }
            else
            {
                printf("Prioriuty command issued\n");
                priority_commands.pop();
            }
        }
        else
        {
            res=FifoCommandScheduler::selectCommand(currentTimestamp);
        }
        return res;
    }

    bool FifoCommandSchedulerWithPriorities::hasCommands()
    {
        return priority_commands.size()>0 || FifoCommandScheduler::hasCommands();
    }
}
