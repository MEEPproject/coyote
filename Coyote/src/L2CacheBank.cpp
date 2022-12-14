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


#include "sparta/utils/SpartaAssert.hpp"
#include "L2CacheBank.hpp"
#include <chrono>

namespace coyote
{
    const char L2CacheBank::name[] = "l2";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    L2CacheBank::L2CacheBank(sparta::TreeNode *node, const L2CacheBankParameterSet *p) :
                    CacheBank(node, p->always_hit, p->writeback, p->miss_latency, p->hit_latency,
                              p->max_outstanding_misses, p->max_outstanding_wbs, false, p->unit_test, p->line_size, p->size_kb,
                              p->associativity, p->lvrf_ways, p->bank_and_tile_offset)
    {
        in_core_req_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(L2CacheBank, getAccess_, std::shared_ptr<Request>));

        in_biu_ack_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(L2CacheBank, sendAck_, std::shared_ptr<CacheRequest>));
	
    }

    void L2CacheBank::scheduleIssueAccess(uint64_t cycle)
    {
        issue_access_event_.schedule(cycle);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive MSS access acknowledge from Bus Interface Unit
    void L2CacheBank::sendAck_(const std::shared_ptr<CacheRequest> & req)
    {
        // do sendAck every cycle
        if(unit_test || getTile()->getArbiter()->hasL2NoCQueueFreeSlot(get_bank_id()))
        {
            sendAckInternal_(req);
        }
        else
        {
            send_ack_event_.preparePayload(req)->schedule(1);
        }
    }


    void L2CacheBank::logCacheRequest(std::shared_ptr<CacheRequest> r)
    {
        switch(r->getType())
        {
            case CacheRequest::AccessType::FETCH:
                logger_->logL2Read(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;

            case CacheRequest::AccessType::LOAD:
                logger_->logL2Read(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;

            case CacheRequest::AccessType::STORE:
                logger_->logL2Write(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;

            case CacheRequest::AccessType::WRITEBACK:
                logger_->logL2Write(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress(), r->getSize());
                break;
        }
    }

    void L2CacheBank::getAccess_(const std::shared_ptr<Request> & req)
    {
        CacheBank::getAccess_(req);
    }

    void L2CacheBank::issueAccess_()
    {
        if(unit_test || getTile()->getArbiter()->hasL2NoCQueueFreeSlot(get_bank_id())) //Trace driven simulation might not have an associated tile
        {
            issueAccessInternal_();
        }
        else
        {
            issue_access_event_.schedule(1); //keep checking every cycle if there is space in the Queue
        }
    }
    
    bool L2CacheBank::handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr) {
        bool hit=CacheBank::handleCacheLookupReq_(mem_access_info_ptr);
        if(trace_)
        {
            if(!hit)
            {
                logger_->logL2Miss(getClock()->currentCycle(), mem_access_info_ptr->getReq()->getCoreId(), mem_access_info_ptr->getReq()->getPC(), mem_access_info_ptr->getReq()->getAddress());
                auto evicted_line_time=eviction_times_.find(mem_access_info_ptr->getReq()->getAddress());
                if(evicted_line_time!=eviction_times_.end())
                {
                    logger_->logMissOnEvicted(getClock()->currentCycle(), mem_access_info_ptr->getReq()->getCoreId(), mem_access_info_ptr->getReq()->getPC(), mem_access_info_ptr->getReq()->getAddress(), getClock()->currentCycle()-evicted_line_time->second);
                    eviction_times_.erase(evicted_line_time);
                }
            }
            else
            {
                logger_->logL2Hit(getClock()->currentCycle(), mem_access_info_ptr->getReq()->getCoreId(), mem_access_info_ptr->getReq()->getPC(), mem_access_info_ptr->getReq()->getAddress());
            }
        }
        return hit;
    }
    
    
    // Reload cache line
    void L2CacheBank::reloadCache_(uint64_t phyAddr, uint16_t bank, CacheRequest::AccessType type, bool is_vector)
    {
        CacheBank::reloadCache_(phyAddr, bank, type, is_vector);
        
        auto l2_cache_line = &l2_cache_->getLineForReplacementWithInvalidCheck(phyAddr);
        if(trace_) {
            if(l2_cache_line->isModified()) {
                logger_->logL2WB(getClock()->currentCycle(), 0, 0, l2_cache_line->getAddr(), getLineSize());
                eviction_times_[l2_cache_line->getAddr()]=getClock()->currentCycle();
            }
            if(l2_cache_line->isValid()) {
                eviction_times_[l2_cache_line->getAddr()]=getClock()->currentCycle();
            }
        }
    }

} // namespace core_example
