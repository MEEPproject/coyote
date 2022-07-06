
#include "sparta/utils/SpartaAssert.hpp"
#include "CacheBank.hpp"
#include "L3CacheBank.hpp"
#include <chrono>

namespace coyote
{
    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    CacheBank::CacheBank(sparta::TreeNode *node, bool always_hit, bool writeback, uint16_t miss_latency, uint16_t hit_latency,
                         uint16_t max_outstanding_misses, uint16_t max_in_flight_wbs, bool busy, bool unit_test, uint64_t line_size, uint64_t size_kb,
                         uint64_t associativity, uint64_t lvrf_ways, uint32_t bank_and_tile_offset) :
        sparta::Unit(node),
        memory_access_allocator(2000, 1000),
        always_hit_(always_hit),
        writeback_(writeback),
        miss_latency_(miss_latency),
        hit_latency_(hit_latency),
        max_outstanding_misses_(max_outstanding_misses),
        busy_(busy),
        in_flight_misses_(max_outstanding_misses, line_size),
        max_in_flight_wbs(max_in_flight_wbs),
        num_in_flight_wbs(0),
        pending_wb(nullptr),
        pending_fetch_requests_(),
        pending_load_requests_(),
        pending_store_requests_(),
        pending_scratchpad_requests_(),
        l2_size_kb_(size_kb),
        l2_associativity_(associativity),
        l2_line_size_(line_size),
        bank_and_tile_offset_(bank_and_tile_offset),
        eviction_times_(),
        unit_test(unit_test)
    {

        // Cache config
        std::unique_ptr<sparta::cache::ReplacementIF> repl(new sparta::cache::TreePLRUReplacement
                                                         (l2_associativity_));
        l2_cache_.reset(new SimpleDL2( getContainer(), l2_size_kb_, l2_line_size_, l2_line_size_*bank_and_tile_offset_, *repl ));

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "CacheBank construct: #" << node->getGroupIdx();
        }
        sparta_assert(lvrf_ways<=associativity, "Cannot disable more ways than the ones available in the cache bank!");

        if(lvrf_ways>0)
        {
            l2_cache_->disableWays(lvrf_ways);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive MSS access acknowledge from Bus Interface Unit
    void CacheBank::sendAckInternal_(const std::shared_ptr<CacheRequest> & req)
    {
        bool was_stalled=(in_flight_misses_.is_full() || pending_wb!=nullptr);

        //For write-through caches, stores are no-write allocate, so no need to reload the cache
        //For write-back, stores are write allocate, so we have to reload the cache
        if((writeback_ && req->getType()!=CacheRequest::AccessType::WRITEBACK) || (!writeback_ && req->getType()!=CacheRequest::AccessType::STORE))
        {
            reloadCache_(calculateLineAddress(req), req->getCacheBank(), req->getType(), req->getProducedByVector());

            auto range_misses=in_flight_misses_.equal_range(req);
            sparta_assert(range_misses.first != range_misses.second, "Got an ack for an unrequested miss\n");

            while(range_misses.first != range_misses.second)
            {
                range_misses.first->second->setServiced();
                //The total time spent by requests is not updated here. This is for acks
                if(!unit_test)
                {
                    out_core_ack_.send(range_misses.first->second);
                }
                range_misses.first++;
            }

            in_flight_misses_.erase(req);
        }
        else if(writeback_ && req->getType()!=CacheRequest::AccessType::STORE)
        {
            if(num_in_flight_wbs==max_in_flight_wbs && pending_wb!=nullptr) //If it was stalled due to WBs
            {
                out_biu_req_.send(pending_wb, 1);                
                pending_wb=nullptr;
            }
            else
            {
                num_in_flight_wbs--;
            }
        }

        if(pending_fetch_requests_.size()+pending_load_requests_.size()+pending_store_requests_.size()+pending_scratchpad_requests_.size()>0)
        {
            //ISSUE EVENT
            if(was_stalled && !busy_ && !in_flight_misses_.is_full() && pending_wb==nullptr)
            {
                busy_=true;
                scheduleIssueAccess(sparta::Clock::Cycle(0));
            }
        }
        else
        {
            busy_ = false;
        }
    }

    void CacheBank::getAccess_(const std::shared_ptr<Request> & req)
    {
        req->setTimestampReachCacheBank(getClock()->currentCycle());
        req->handle(this);
    }

    void CacheBank::issueAccessInternal_()
    {
        bool stall=false;
        if(pending_scratchpad_requests_.size()>0) //Scratchpad requests are handled first
        {
            const std::shared_ptr<ScratchpadRequest> s=pending_scratchpad_requests_.front();
            pending_scratchpad_requests_.pop_front(); //We always hit in the scratchpad. Nothing to check.

            if(s->getCommand()==ScratchpadRequest::ScratchpadCommand::ALLOCATE)
            {
                l2_cache_->disableWays(s->getSize());
            }
            else if(s->getCommand()==ScratchpadRequest::ScratchpadCommand::FREE)
            {
                l2_cache_->enableWays(s->getSize());
            }
            s->setServiced();
            out_core_ack_.send(s, hit_latency_);
        }
        else
        {
            MemoryAccessInfoPtr m;
            if(pending_fetch_requests_.size()>0)
            {
                m=sparta::allocate_sparta_shared_pointer<MemoryAccessInfo>(memory_access_allocator, pending_fetch_requests_.front());   
                pending_fetch_requests_.pop_front();
            }
            else if(pending_load_requests_.size()>0)
            {
                m=sparta::allocate_sparta_shared_pointer<MemoryAccessInfo>(memory_access_allocator, pending_load_requests_.front());  
                pending_load_requests_.pop_front();
            }
            else if(pending_store_requests_.size()>0)
            {
                m=sparta::allocate_sparta_shared_pointer<MemoryAccessInfo>(memory_access_allocator, pending_store_requests_.front());
                pending_store_requests_.pop_front();
            }
 
            handleCacheLookupReq_(m);

            if(in_flight_misses_.is_full() || pending_wb!=nullptr)
            {
                stall=true;
                count_stall_++;
            }
        }

        if(!stall && pending_wb==nullptr && (pending_fetch_requests_.size()+pending_load_requests_.size()+pending_store_requests_.size()+pending_scratchpad_requests_.size()>0)) //IF THERE ARE PENDING REQUESTS, SCHEDULE NEXT ISSUE
        {
           scheduleIssueAccess(sparta::Clock::Cycle(hit_latency_)); //THE NEXT ACCESS CAN BE ISSUED AFTER THE SEARCH IN THE L2 HAS FINISHED (EITHER HIT OR MISS)
        }
        else
        {
            busy_=false;
        }
    }

    // Handle cache access request
    bool CacheBank::handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        bool CACHE_HIT=true;
        if(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::WRITEBACK)
        {
            reloadCache_(calculateLineAddress(mem_access_info_ptr->getReq()), mem_access_info_ptr->getReq()->getCacheBank(), mem_access_info_ptr->getReq()->getType(), mem_access_info_ptr->getReq()->getProducedByVector());
            CACHE_HIT=true;
        }
        else if(writeback_ || mem_access_info_ptr->getReq()->getType()!=CacheRequest::AccessType::STORE)
        {
            // Access cache, and check cache hit or miss
            CACHE_HIT = cacheLookup_(mem_access_info_ptr);
        }

        if (CACHE_HIT)
        {
            mem_access_info_ptr->getReq()->setServiced();
            if(!unit_test)
            {
                out_core_ack_.send(mem_access_info_ptr->getReq(), hit_latency_);
            }
            total_time_spent_by_requests_=total_time_spent_by_requests_+(getClock()->currentCycle()+hit_latency_-mem_access_info_ptr->getReq()->getTimestampReachCacheBank());
            if(!writeback_ && mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::STORE)
            {
                out_biu_req_.send(mem_access_info_ptr->getReq(), sparta::Clock::Cycle(hit_latency_));
            }
        }
        else 
        {
            //count_cache_misses_++;

            // Update memory access info


            if (cache_busy_ == false) {
                // Cache is now busy_, no more CACHE MISS can be handled, RESET required on finish
                //cache_busy_ = true;
                // Keep record of the current CACHE MISS instruction

                // NOTE:
                // cache_busy_ RESET could be done:
                // as early as port-driven event for this miss finish, and
                // as late as cache reload event for this miss finish.

                bool already_pending=false;

                //For writethrough cache, dont track stores in inflight
                if(writeback_ || (mem_access_info_ptr->getReq()->getType() != CacheRequest::AccessType::STORE))
                {
                    already_pending=in_flight_misses_.contains(mem_access_info_ptr->getReq());
                    in_flight_misses_.insert(mem_access_info_ptr->getReq());
                }

                //MISSES ON LOADS AND FETCHES ARE ONLY FORWARDED IF THE LINE IS NOT ALREADY PENDING
                if(!already_pending)
                {
                    if(mem_access_info_ptr->getReq()->getProducedByVector())
                    {
                        count_vector_misses_++;
                    }
                    else
                    {
                        count_non_vector_misses_++;
                    }
                    std::shared_ptr<coyote::CacheRequest> cache_req = mem_access_info_ptr->getReq();
                    out_biu_req_.send(cache_req, sparta::Clock::Cycle(miss_latency_));
                    total_time_spent_by_requests_=total_time_spent_by_requests_+(getClock()->currentCycle()+miss_latency_-mem_access_info_ptr->getReq()->getTimestampReachCacheBank());
                }
                else
                {
                    //For writethrough caches, all the writes are sent to lower level cache
                    if(!writeback_ && mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::STORE)
                    {
                        out_biu_req_.send(mem_access_info_ptr->getReq(), sparta::Clock::Cycle(miss_latency_));
                    }
                    else
                    {
                        count_misses_on_already_pending_++;
                        if(mem_access_info_ptr->getReq()->getProducedByVector())
                        {
                            count_vector_misses_++;
                        }
                        else
                        {
                            count_non_vector_misses_++;
                        }
                    }
                    //Nothing more to do until the ack arrives. Do not update the stats
                    total_time_spent_by_requests_=total_time_spent_by_requests_+(getClock()->currentCycle()-mem_access_info_ptr->getReq()->getTimestampReachCacheBank());
                }

                if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
                    info_logger_ << "Cache is trying to drive BIU request port!";
                }
            }
            else {

                if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
                    info_logger_ << "Cache miss cannot be served right now due to another outstanding one!";
                }
            }
        }
        return CACHE_HIT;
    }


    // Access Cache
    bool CacheBank::cacheLookup_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        uint64_t phyAddr = mem_access_info_ptr->getRAdr();

        bool cache_hit = false;

        if (always_hit_) {
            cache_hit = true;
        }
        else {
            auto cache_line = l2_cache_->peekLine(phyAddr);
            cache_hit = (cache_line != nullptr) && cache_line->isValid();

            if (cache_hit) {
                l2_cache_->touchMRU(*cache_line);

                if(mem_access_info_ptr->getReq()->getProducedByVector())
                {
                    cache_line->setAccessedByVector(true);
                }
                else
                {
                    cache_line->setAccessedByNonVector(true);
                }

                //SET DIRTY BIT IF NECESSARY
                if((mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::STORE || mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::WRITEBACK) && writeback_)
                 {
                    cache_line->setModified(true); //send the block to memory for write through l2
                }
            }
        }

        if (SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            if (always_hit_) {
                info_logger_ << "Cache HIT all the time: phyAddr=0x" << std::hex << phyAddr;
            }
            else if (cache_hit) {
                info_logger_ << "Cache HIT: phyAddr=0x" << std::hex << phyAddr;
            }
            else {
                info_logger_ << "Cache MISS: phyAddr=0x" << std::hex << phyAddr;
            }
        }

        return cache_hit;
    }

    // Reload cache line
    void CacheBank::reloadCache_(uint64_t phyAddr, uint16_t bank, CacheRequest::AccessType type, bool is_vector)
    {
        auto l2_cache_line = &l2_cache_->getLineForReplacementWithInvalidCheck(phyAddr);

        //If the line is dirty, send a writeback to the memory
        if(l2_cache_line->isModified())
        {
            std::shared_ptr<CacheRequest> cache_req = std::make_shared<coyote::CacheRequest> (l2_cache_line->getAddr(), CacheRequest::AccessType::WRITEBACK, 0, getClock()->currentCycle(), 0);
            cache_req->setCacheBank(bank);
            cache_req->setSize(l2_line_size_);
            if(num_in_flight_wbs<max_in_flight_wbs)
            {
                out_biu_req_.send(cache_req, 1);
                count_wbs_+=1;
                num_in_flight_wbs++;
            }
            else
            {
                pending_wb=cache_req;
            }
        }

        if(l2_cache_line->isValid())
        {
            if(is_vector)
            {
                if(l2_cache_line->getAccessedByVector() && l2_cache_line->getAccessedByNonVector())
                {
                    count_vector_evicts_mixed_++;
                }
                else if(l2_cache_line->getAccessedByVector())
                {
                    count_vector_evicts_vector_++; 
                }
                else
                {
                    count_vector_evicts_non_vector_++;
                }
            }
            else
            {
                if(l2_cache_line->getAccessedByVector() && l2_cache_line->getAccessedByNonVector())
                {
                    count_non_vector_evicts_mixed_++;
                }
                else if(l2_cache_line->getAccessedByVector())
                {
                    count_non_vector_evicts_vector_++; 
                }
                else
                {
                    count_non_vector_evicts_non_vector_++;
                }
            }
        }

        l2_cache_->allocateWithMRUUpdate(*l2_cache_line, phyAddr);
                
        if(is_vector)
        {
            l2_cache_line->setAccessedByVector(true);
        }
        else
        {
            l2_cache_line->setAccessedByNonVector(true);
        }

        if(type == CacheRequest::AccessType::WRITEBACK || type == CacheRequest::AccessType::STORE)
            l2_cache_line->setModified(true); //Send the block to memory for write through L2

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "Cache reload complete!";
        }
    }

    uint64_t CacheBank::calculateLineAddress(std::shared_ptr<CacheRequest> r)
    {
        return (r->getAddress() >> l2_line_size_) << l2_line_size_;
    }

    void CacheBank::handle(std::shared_ptr<coyote::CacheRequest> r)
    {
        if(r->isServiced())
        {
            sendAckInternal_(r);
        }
        else
        {
            if(trace_)
            {
                logCacheRequest(r);
            }

            if(r->getType()==CacheRequest::AccessType::LOAD || r->getType()==CacheRequest::AccessType::FETCH)
            {
                if(r->getProducedByVector())
                {
                    count_cache_reads_vector_+=1;
                }
                else
                {
                    count_cache_reads_non_vector_+=1;
                }
            }
            else
            {
                if(r->getProducedByVector())
                {
                    count_cache_writes_vector_+=1;
                }
                else
                {
                    count_cache_writes_non_vector_+=1;
                }
            }

            bool hit_on_store=false;
            if(r->getType()==CacheRequest::AccessType::LOAD)
            {
                //CHECK FOR HITS ON PENDING WRITEBACK. STORES ARE NOT CHECKED. YOU NEED TO LOAD A WHOLE LINE, NOT JUST A WORD.
                for (std::list<std::shared_ptr<CacheRequest>>::reverse_iterator rit=pending_store_requests_.rbegin(); rit!=pending_store_requests_.rend() && !hit_on_store; ++rit)
                {
                    hit_on_store=((*rit)->getType()==CacheRequest::AccessType::WRITEBACK && calculateLineAddress(*rit)==calculateLineAddress(r));
                }
            }

            if(hit_on_store)
            {
                //AUTO HIT
                r->setServiced();
                if(!unit_test)
                {
                    out_core_ack_.send(r,1);
                }
                total_time_spent_by_requests_=total_time_spent_by_requests_+(getClock()->currentCycle()+1-r->getTimestampReachCacheBank());
                count_hit_on_store_++;
            }
            else
            {
                switch(r->getType())
                {
                    case CacheRequest::AccessType::FETCH:
                        pending_fetch_requests_.push_back(r);
                        break;

                    case CacheRequest::AccessType::LOAD:
                        pending_load_requests_.push_back(r);
                        break;

                    case CacheRequest::AccessType::STORE:
                        pending_store_requests_.push_back(r);
                        break;

                    case CacheRequest::AccessType::WRITEBACK:
                        pending_store_requests_.push_back(r);
                        break;
                }

                if(!busy_ && !in_flight_misses_.is_full() && pending_wb==nullptr)
                {
                    busy_=true;
                    //ISSUE EVENT
                    scheduleIssueAccess(sparta::Clock::Cycle(0));
                }
            }
        }
    }

    void CacheBank::handle(std::shared_ptr<coyote::ScratchpadRequest> r)
    {
        count_scratchpad_requests_+=1;
        pending_scratchpad_requests_.push_back(r);
        if(!busy_)
        {
            busy_=true;
            scheduleIssueAccess(sparta::Clock::Cycle(0));
        }
    }

    void CacheBank::putEvent(const std::shared_ptr<Event> & ev)
    {
        ev->handle(this);
    }

} // namespace core_example
