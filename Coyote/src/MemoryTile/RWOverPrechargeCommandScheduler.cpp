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

#include "RWOverPrechargeCommandScheduler.hpp"
#include "CacheRequest.hpp"

namespace coyote
{
    RWOverPrechargeCommandScheduler::RWOverPrechargeCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : CommandSchedulerIF(latencies, num_banks), reads_and_writes(), precharges_and_activates(){}

    void RWOverPrechargeCommandScheduler::addCommand(std::shared_ptr<BankCommand> b)
    {
        printf("Adding command for address %#lx\n", b->getRequest()->getAddress());
        if(b->getType()==BankCommand::CommandType::READ || b->getType()==BankCommand::CommandType::WRITE)
        {
            reads_and_writes.push(b);
        }
        else
        {
            precharges_and_activates.push(b);
        }
    }

    std::shared_ptr<BankCommand> RWOverPrechargeCommandScheduler::selectCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res;
        if(reads_and_writes.size()>0)
        {
            res=reads_and_writes.front();
        }
        else
        {
            res=precharges_and_activates.front();
        }
        if(checkTiming(res, currentTimestamp))
        {
            if(reads_and_writes.size()>0)
            {
                reads_and_writes.pop();
            }
            else
            {
                precharges_and_activates.pop();
            }
        }
        else
        {
            res=nullptr;
        }
        return res;
    }

    bool RWOverPrechargeCommandScheduler::hasCommands()
    {
        return reads_and_writes.size()+precharges_and_activates.size()>0;
    }
}
