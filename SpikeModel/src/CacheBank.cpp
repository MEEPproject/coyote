
#include "sparta/utils/SpartaAssert.hpp"
#include "CacheBank.hpp"
#include "L3CacheBank.hpp"
#include <chrono>

namespace spike_model
{
    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    CacheBank::CacheBank(sparta::TreeNode *node, bool always_hit, uint16_t miss_latency, uint16_t hit_latency,
                         uint16_t max_outstanding_misses, uint16_t max_in_flight_wbs, bool busy, uint64_t line_size, uint64_t size_kb,
                         uint64_t associativity, uint32_t bank_and_tile_offset) :
        sparta::Unit(node),
        memory_access_allocator(2000, 1000),
        always_hit_(always_hit),
        miss_latency_(miss_latency),
        hit_latency_(hit_latency),
        max_outstanding_misses_(max_outstanding_misses),
        busy_(busy),
        in_flight_reads_(max_outstanding_misses, line_size),
        max_in_flight_wbs(max_in_flight_wbs),
        num_in_flight_wbs(0),
        pending_wb(nullptr),
        pending_fetch_requests_(),
        pending_load_requests_(),
        pending_store_requests_(),
        pending_scratchpad_requests_(),
        eviction_times_(),
        l2_size_kb_(size_kb),
        l2_associativity_(associativity),
        l2_line_size_(line_size),
        bank_and_tile_offset_(bank_and_tile_offset)
    {

        // Cache config
        std::unique_ptr<sparta::cache::ReplacementIF> repl(new sparta::cache::TreePLRUReplacement
                                                         (l2_associativity_));
        l2_cache_.reset(new SimpleDL1( getContainer(), l2_size_kb_, l2_line_size_, l2_line_size_*bank_and_tile_offset_, *repl ));

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "CacheBank construct: #" << node->getGroupIdx();
        }

    }

    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive MSS access acknowledge from Bus Interface Unit
    void CacheBank::sendAckInternal_(const std::shared_ptr<CacheRequest> & req)
    {
        bool was_stalled=(in_flight_reads_.is_full() || pending_wb!=nullptr);
        
        if(req->getType()!=CacheRequest::AccessType::WRITEBACK)
        {
            reloadCache_(calculateLineAddress(req), req->getCacheBank());

            if(req->getType()==CacheRequest::AccessType::LOAD || req->getType()==CacheRequest::AccessType::FETCH)
            {
                auto range_misses=in_flight_reads_.equal_range(req);

                sparta_assert(range_misses.first != range_misses.second, "Got an ack for an unrequested miss\n");

                while(range_misses.first != range_misses.second)
                {
                    range_misses.first->second->setServiced();
                    //The total time spent by requests is not updated here. This is for acks
                    out_core_ack_.send(range_misses.first->second);
                    range_misses.first++;
                }

                in_flight_reads_.erase(req);
            }
            else //STORE
            {
                out_core_ack_.send(req);
            }
        }
        else
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
            if(was_stalled && !busy_ && !in_flight_reads_.is_full() && pending_wb==nullptr)
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

            if(in_flight_reads_.is_full())
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

        bool CACHE_HIT;
        if(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::WRITEBACK)
        {
            reloadCache_(calculateLineAddress(mem_access_info_ptr->getReq()), mem_access_info_ptr->getReq()->getCacheBank());
            CACHE_HIT=true;
        }
        else
        {
            // Access cache, and check cache hit or miss
            CACHE_HIT = cacheLookup_(mem_access_info_ptr);
        }

        if (CACHE_HIT) {
                mem_access_info_ptr->getReq()->setServiced();
               	out_core_ack_.send(mem_access_info_ptr->getReq(), hit_latency_);
                total_time_spent_by_requests_=total_time_spent_by_requests_+(getClock()->currentCycle()+hit_latency_-mem_access_info_ptr->getReq()->getTimestampReachCacheBank());
        }
        else {
            count_cache_misses_++;

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

                if(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::LOAD || mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::FETCH)
                {
                    already_pending=in_flight_reads_.contains(mem_access_info_ptr->getReq());
                    in_flight_reads_.insert(mem_access_info_ptr->getReq());
                }

                //MISSES ON LOADS AND FETCHES ARE ONLY FORWARDED IF THE LINE IS NOT ALREADY PENDING
                if(!(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::LOAD || mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::FETCH) || !already_pending)
                {
                    std::shared_ptr<spike_model::CacheRequest> cache_req = mem_access_info_ptr->getReq();

                    out_biu_req_.send(cache_req, sparta::Clock::Cycle(miss_latency_));
                    total_time_spent_by_requests_=total_time_spent_by_requests_+(getClock()->currentCycle()+miss_latency_-mem_access_info_ptr->getReq()->getTimestampReachCacheBank());

                    //NO FURTHER ACTION IS NEEDED
/*                    if(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::STORE || mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::WRITEBACK)
                    {
                        mem_access_info_ptr->getReq()->setServiced();
                        //This reply was already accounted for regarding the stats in the outer "if"
                        out_core_ack_.send(mem_access_info_ptr->getReq());
                    }*/
                }
                else
                {
                    count_misses_on_already_pending_++;
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
                //SET DIRTY BIT IF NECESSARY
                if(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::STORE || mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::WRITEBACK)
                {
                    cache_line->setModified(true);
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
    void CacheBank::reloadCache_(uint64_t phyAddr, uint16_t bank)
    {
        auto l2_cache_line = &l2_cache_->getLineForReplacementWithInvalidCheck(phyAddr);

        //If the line is dirty, send a writeback to the memory
        if(l2_cache_line->isModified())
        {
            std::shared_ptr<CacheRequest> cache_req = std::make_shared<spike_model::CacheRequest> (l2_cache_line->getAddr(), CacheRequest::AccessType::WRITEBACK, 0, getClock()->currentCycle(), 0);
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

        l2_cache_->allocateWithMRUUpdate(*l2_cache_line, phyAddr);
        
        l2_cache_line->setModified(true);

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "Cache reload complete!";
        }
    }

    uint64_t CacheBank::calculateLineAddress(std::shared_ptr<CacheRequest> r)
    {
        return (r->getAddress() >> l2_line_size_) << l2_line_size_;
    }

    void CacheBank::handle(std::shared_ptr<spike_model::CacheRequest> r)
    {
        if(r->isServiced())
        {
            sendAckInternal_(r);
        }
        else
        {
            count_cache_requests_+=1;

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
                out_core_ack_.send(r,1);
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

                if(!busy_ && !in_flight_reads_.is_full() && pending_wb==nullptr)
                {
                    busy_=true;
                    //ISSUE EVENT
                    scheduleIssueAccess(sparta::Clock::Cycle(0));
                }
            }
        }
    }

    void CacheBank::handle(std::shared_ptr<spike_model::ScratchpadRequest> r)
    {
        count_scratchpad_requests_+=1;
        pending_scratchpad_requests_.push_back(r);
        if(!busy_)
        {
            busy_=true;
            scheduleIssueAccess(sparta::Clock::Cycle(0));
        }
    }


} // namespace core_example
