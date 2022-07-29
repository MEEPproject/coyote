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

#include "OldestRWOverPrechargeCommandScheduler.hpp"
#include "CacheRequest.hpp"

namespace coyote
{
    OldestRWOverPrechargeCommandScheduler::OldestRWOverPrechargeCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : CommandSchedulerIF(latencies, num_banks), reads_and_writes(), precharges_and_activates(){}

    void OldestRWOverPrechargeCommandScheduler::addCommand(std::shared_ptr<BankCommand> b)
    {
        printf("Adding command for address %#lx\n", b->getRequest()->getAddress());
        if(b->getType()==BankCommand::CommandType::READ || b->getType()==BankCommand::CommandType::WRITE)
        {
            reads_and_writes.push_back(b);
        }
        else
        {
            precharges_and_activates.push_back(b);
        }
    }

    std::shared_ptr<BankCommand> OldestRWOverPrechargeCommandScheduler::selectCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res=nullptr;
       
        std::list<std::shared_ptr<BankCommand>>::iterator it;
        it = reads_and_writes.begin();
        while (it != reads_and_writes.end() && res==nullptr)
        {
            if(checkTiming(*it, currentTimestamp))
            {
                res=*it;
                reads_and_writes.erase(it);
            }
            else
            {
                it++;
            }
        }
       
        if(res==nullptr)
        {
            it = precharges_and_activates.begin();
            while (it != precharges_and_activates.end() && res==nullptr)
            {
                if(checkTiming(*it, currentTimestamp))
                {
                    res=*it;
                    precharges_and_activates.erase(it);
                }
                else
                {
                    it++;
                }
            }

        }

        return res;
    }

    bool OldestRWOverPrechargeCommandScheduler::hasCommands()
    {
        return reads_and_writes.size()+precharges_and_activates.size()>0;
    }
}
