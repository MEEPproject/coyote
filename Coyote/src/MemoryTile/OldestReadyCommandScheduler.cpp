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

#include "OldestReadyCommandScheduler.hpp"

namespace coyote
{
    OldestReadyCommandScheduler::OldestReadyCommandScheduler(std::shared_ptr<std::vector<uint64_t>> latencies, uint16_t num_banks) : CommandSchedulerIF(latencies, num_banks), commands(){}

    void OldestReadyCommandScheduler::addCommand(std::shared_ptr<BankCommand> b)
    {
        commands.push_back(b);
    }

    std::shared_ptr<BankCommand> OldestReadyCommandScheduler::selectCommand(uint64_t currentTimestamp)
    {
        std::shared_ptr<BankCommand> res=nullptr;

        std::list<std::shared_ptr<BankCommand>>::iterator it = commands.begin();
        while (it != commands.end() && res==nullptr)
        {
            if(checkTiming(*it, currentTimestamp))
            {
                res=*it;
                commands.erase(it);
            }
            else
            {
                it++;
            }
        }
        return res;
    }

    bool OldestReadyCommandScheduler::hasCommands()
    {
        return commands.size()>0;
    }
}
