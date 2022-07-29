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

#include "CacheRequest.hpp"
#include "BankCommand.hpp"
#include "MemoryAccessSchedulerIF.hpp"

namespace coyote
{

    MemoryAccessSchedulerIF::MemoryAccessSchedulerIF(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate) : 
        write_allocate(write_allocate),
        pending_command(num_banks, false),
        banks(b)
    {}

    std::shared_ptr<BankCommand> MemoryAccessSchedulerIF::getCommand(uint64_t bank)
    {
        std::shared_ptr<CacheRequest> req=getRequest(bank);

        uint64_t row_to_schedule=req->getRow();

        std::shared_ptr<BankCommand> com=nullptr;
        if((*banks)[bank]->isOpen()) 
        {
            if((*banks)[bank]->getOpenRow()==row_to_schedule) 
            {
                com=getAccessCommand_(req, bank);
            } 
            else 
            {
                com=std::make_shared<BankCommand>(BankCommand::CommandType::PRECHARGE, bank, 0, req);
            }
        } 
        else 
        {
            com=std::make_shared<BankCommand>(BankCommand::CommandType::ACTIVATE, bank, row_to_schedule, req);
        }

        pending_command[bank]=true;

        checkRequestCompletion(com);
        
        return com;
    }
    
    void MemoryAccessSchedulerIF::checkRequestCompletion(std::shared_ptr<BankCommand> c)
    {
        bool res=false;
        switch(c->getType())
        {
            case BankCommand::CommandType::READ:
            {
                std::shared_ptr<CacheRequest> req=c->getRequest();

                // If it is a fetch, load or store (with write_allocate) and all the associated commands have been submitted
                if((req->getType()==CacheRequest::AccessType::LOAD || req->getType()==CacheRequest::AccessType::FETCH || (req->getType()==CacheRequest::AccessType::STORE && write_allocate && req->isAllocating())) && req->getSizeRequestedToMemory()>=req->getSize())
                {
                    notifyRequestCompletion(req);
                    res=true;
                }
                break;
            }

            case BankCommand::CommandType::WRITE:
            {
                std::shared_ptr<CacheRequest> req=c->getRequest();

                if((!write_allocate || c->getRequest()->getType()==CacheRequest::AccessType::WRITEBACK) && req->getSizeRequestedToMemory()>=req->getSize())
                {
                    notifyRequestCompletion(req);
                    res=true;
                }
                break;
            }

            default:
            {
                break;
            }
        }
        if(res)
        {
            c->setCompletesRequest();
        }
    }

    std::shared_ptr<BankCommand> MemoryAccessSchedulerIF::getAccessCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        std::shared_ptr<BankCommand> res_command=nullptr;

        uint64_t column_to_schedule=req->getCol();
        if((req->getType()==CacheRequest::AccessType::STORE && (!req->isAllocating() || !write_allocate)) || req->getType()==CacheRequest::AccessType::WRITEBACK)
        {
            res_command=std::make_shared<BankCommand>(BankCommand::CommandType::WRITE, bank, column_to_schedule, req);
        }
        else
        {
            res_command=std::make_shared<BankCommand>(BankCommand::CommandType::READ, bank, column_to_schedule, req);
        }
        req->increaseSizeRequestedToMemory((*banks)[0]->getBurstSize());
        return res_command;
    }

    void MemoryAccessSchedulerIF::notifyCommandSubmission(std::shared_ptr<BankCommand> c)
    {
        pending_command[c->getDestinationBank()]=false;
        std::shared_ptr<CacheRequest> req=c->getRequest();
        if(req->getType()==CacheRequest::AccessType::STORE && write_allocate && req->getSizeRequestedToMemory()>=req->getSize() && !req->isAllocating())
        {
            req->setAllocate();
        }
        rescheduleBank(c->getDestinationBank());
    }
}
