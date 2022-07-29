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

#include "GreedyFifoRrMemoryAccessScheduler.hpp"

namespace coyote
{
    GreedyFifoRrMemoryAccessScheduler::GreedyFifoRrMemoryAccessScheduler(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate) : FifoRrMemoryAccessScheduler(b, num_banks, write_allocate){}

             
    void GreedyFifoRrMemoryAccessScheduler::rescheduleBank(uint64_t bank)
    {
        if(request_queues[bank].size()>0)
        {
            printf("The size is %lu\n", request_queues[bank].size());
            std::shared_ptr<CacheRequest> req=getRequest(bank);

            uint64_t row_to_schedule=req->getRow();

            printf("The open row is %lu and the one to schedule is %lu\n", (*banks)[bank]->getOpenRow(), row_to_schedule);

            //if((*banks)[bank]->isOpen() && req->getSizeRequestedToMemory()<req->getSize() && (*banks)[bank]->getOpenRow()==row_to_schedule) 
            if(!(*banks)[bank]->isOpen() || (*banks)[bank]->getOpenRow()!=row_to_schedule) 
            {
                printf("Greedily adding bank %lu!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", bank);
                banks_to_schedule.push_front(bank);
                //banks_to_schedule.push_back(bank);
            }
            else
            {
                banks_to_schedule.push_back(bank);
            }
        }
    }
    
    std::shared_ptr<BankCommand> GreedyFifoRrMemoryAccessScheduler::getCommand(uint64_t bank)
    {
        std::shared_ptr<BankCommand> res=MemoryAccessSchedulerIF::getCommand(bank);
        //if(res!=nullptr && res->getRequest()->getSizeRequestedToMemory()>0)
        if(res!=nullptr && (res->getType()==BankCommand::CommandType::ACTIVATE || res->getType()==BankCommand::CommandType::PRECHARGE))
        {
            res->setHighPriority();
        }
        return res;
    }
}
