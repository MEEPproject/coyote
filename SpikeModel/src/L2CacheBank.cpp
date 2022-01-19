
#include "sparta/utils/SpartaAssert.hpp"
#include "L2CacheBank.hpp"
#include <chrono>

namespace spike_model
{
    const char L2CacheBank::name[] = "l2";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    L2CacheBank::L2CacheBank(sparta::TreeNode *node, const L2CacheBankParameterSet *p) :
                    CacheBank(node, p->always_hit, p->miss_latency, p->hit_latency,
                              p->max_outstanding_misses, p->max_outstanding_wbs, false, p->line_size, p->size_kb,
                              p->associativity, p->bank_and_tile_offset)
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
        if(getTile()->getArbiter()->hasL2NoCQueueFreeSlot(get_bank_id()))
        {
            sendAckInternal_(req);
        }
        else
        {
            send_ack_event_.preparePayload(req)->schedule(1);
        }
    }

    void L2CacheBank::getAccess_(const std::shared_ptr<Request> & req)
    {
        CacheBank::getAccess_(req);
    }

    void L2CacheBank::issueAccess_()
    {
        if(getTile()->getArbiter()->hasL2NoCQueueFreeSlot(get_bank_id()))
        {
            issueAccessInternal_();
        }
        else
        {
            issue_access_event_.schedule(1); //keep checking every cycle if there is space in the Queue
        }
    }
    
    bool L2CacheBank::handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr) {
        if(trace_)
        {
            logger_->logL2Miss(getClock()->currentCycle(), mem_access_info_ptr->getReq()->getCoreId(), mem_access_info_ptr->getReq()->getPC(), mem_access_info_ptr->getReq()->getAddress());
            auto evicted_line_time=eviction_times_.find(mem_access_info_ptr->getReq()->getAddress());
            if(evicted_line_time!=eviction_times_.end())
            {
                logger_->logMissOnEvicted(getClock()->currentCycle(), mem_access_info_ptr->getReq()->getCoreId(), mem_access_info_ptr->getReq()->getPC(), mem_access_info_ptr->getReq()->getAddress(), getClock()->currentCycle()-evicted_line_time->second);
                eviction_times_.erase(evicted_line_time);
            }
        }
        return CacheBank::handleCacheLookupReq_(mem_access_info_ptr);
    }
    
    
    // Reload cache line
    void L2CacheBank::reloadCache_(uint64_t phyAddr, uint16_t bank)
    {
        CacheBank::reloadCache_(phyAddr, bank);
        
        auto l2_cache_line = &l2_cache_->getLineForReplacementWithInvalidCheck(phyAddr);
        if(trace_) {
            if(l2_cache_line->isModified()) {
                logger_->logL2WB(getClock()->currentCycle(), 0, 0, l2_cache_line->getAddr());
                eviction_times_[l2_cache_line->getAddr()]=getClock()->currentCycle();
            }
            if(l2_cache_line->isValid()) {
                eviction_times_[l2_cache_line->getAddr()]=getClock()->currentCycle();
            }
        }
    }

} // namespace core_example
