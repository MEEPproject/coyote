
#include "sparta/utils/SpartaAssert.hpp"
#include "CacheBank.hpp"
#include <chrono>

namespace spike_model
{
    const char CacheBank::name[] = "l2";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    CacheBank::CacheBank(sparta::TreeNode *node, const CacheBankParameterSet *p) :
        sparta::Unit(node),
        memory_access_allocator(2000, 1000),
        always_hit_(p->always_hit),
        miss_latency_(p->miss_latency),
        hit_latency_(p->hit_latency),
        max_outstanding_misses_(p->max_outstanding_misses),
        busy_(false),
        in_flight_reads_(max_outstanding_misses_, p->line_size),
        pending_fetch_requests_(),
        pending_load_requests_(),
        pending_store_requests_(),
        pending_scratchpad_requests_(),
        l2_size_kb_ (p->size_kb),
        l2_associativity_ (p->associativity),
        l2_line_size_ (p->line_size)
    {

        in_core_req_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(CacheBank, getAccess_, std::shared_ptr<Request>));

        in_biu_ack_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(CacheBank, sendAck_, std::shared_ptr<CacheRequest>));


        // DL1 cache config
        std::unique_ptr<sparta::cache::ReplacementIF> repl(new sparta::cache::TreePLRUReplacement
                                                         (l2_associativity_));
        l2_cache_.reset(new SimpleDL1( getContainer(), l2_size_kb_, l2_line_size_, *repl ));

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "CacheBank construct: #" << node->getGroupIdx();
        }

    }


    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive MSS access acknowledge from Bus Interface Unit
    void CacheBank::sendAck_(const std::shared_ptr<CacheRequest> & req)
    {
        bool was_stalled=in_flight_reads_.is_full();

        req->setServiced();

        reloadCache_(calculateLineAddress(req));
        if(req->getType()==CacheRequest::AccessType::LOAD || req->getType()==CacheRequest::AccessType::FETCH)
        {
            auto range_misses=in_flight_reads_.equal_range(req);

            sparta_assert(range_misses.first != range_misses.second, "Got an ack for an unrequested miss\n");

            while(range_misses.first != range_misses.second)
            {
                range_misses.first->second->setServiced();
                out_core_ack_.send(range_misses.first->second);
                range_misses.first++;
            }

            in_flight_reads_.erase(req);
        
        }
        if(pending_fetch_requests_.size()+pending_load_requests_.size()+pending_store_requests_.size()+pending_scratchpad_requests_.size()>0)
        {
            //ISSUE EVENT
            if(was_stalled && !busy_)
            {
                busy_=true;
                issue_access_event_.schedule(sparta::Clock::Cycle(0));
            }
        }
        else
        {
            busy_=false;
        }
    }   


    void CacheBank::getAccess_(const std::shared_ptr<Request> & req)
    {
        req->handle(this);
    }

    void CacheBank::issueAccess_()
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
            
            //if(m!=NULL) // m might be null if two events get scheduled for the same cycle, due to an event with hit_latency delay and a miss response mathing
            //{
                handleCacheLookupReq_(m);

                if(in_flight_reads_.is_full())
                {
                    stall=true;
                    count_stall_++;
                }
            //}
        }

        if(!stall && (pending_fetch_requests_.size()+pending_load_requests_.size()+pending_store_requests_.size()+pending_scratchpad_requests_.size()>0)) //IF THERE ARE PENDING REQUESTS, SCHEDULE NEXT ISSUE
        {
            issue_access_event_.schedule(sparta::Clock::Cycle(hit_latency_)); //THE NEXT ACCESS CAN BE ISSUED AFTER THE SEARCH IN THE L2 HAS FINISHED (EITHER HIT OR MISS)
        }
        else
        {
            busy_=false;
        }

    }

    // Handle cache access request
    bool CacheBank::handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        // Access cache, and check cache hit or miss
        const bool CACHE_HIT = cacheLookup_(mem_access_info_ptr);


        if (CACHE_HIT) {
                mem_access_info_ptr->getReq()->setServiced();
               	out_core_ack_.send(mem_access_info_ptr->getReq(), hit_latency_);
        }
        else {
            count_cache_misses_++;
            // Update memory access info
            if(trace_)
            {
                logger_.logL2Miss(getClock()->currentCycle(), mem_access_info_ptr->getReq()->getCoreId(), mem_access_info_ptr->getReq()->getPC(), mem_access_info_ptr->getReq()->getAddress());
            }

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
                    out_biu_req_.send(mem_access_info_ptr->getReq(), sparta::Clock::Cycle(miss_latency_));

                    //NO FURTHER ACTION IS NEEDED
                    if(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::STORE || mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::WRITEBACK)
                    {
                        mem_access_info_ptr->getReq()->setServiced();
                        out_core_ack_.send(mem_access_info_ptr->getReq());
                    }
                }
                else
                {
                    count_misses_on_already_pending_++;
                }

                //THE CACHE IS WRITE ALLOCATE, SO MISSES ON WRITEBACKS NEED TO KEEP THE LINE APPART FROM FORWARDING IT
                if(mem_access_info_ptr->getReq()->getType()==CacheRequest::AccessType::WRITEBACK)
                {
                    reloadCache_(calculateLineAddress(mem_access_info_ptr->getReq()));
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
                info_logger_ << "DL1 Cache HIT all the time: phyAddr=0x" << std::hex << phyAddr;
            }
            else if (cache_hit) {
                info_logger_ << "DL1 Cache HIT: phyAddr=0x" << std::hex << phyAddr;
            }
            else {
                info_logger_ << "DL1 Cache MISS: phyAddr=0x" << std::hex << phyAddr;
            }
        }

        return cache_hit;
    }

    // Reload cache line
    void CacheBank::reloadCache_(uint64_t phyAddr)
    {
        auto l2_cache_line = &l2_cache_->getLineForReplacementWithInvalidCheck(phyAddr);

        //If the line is dirty, send a writeback to the memory
        if(l2_cache_line->isModified())
        {
            out_biu_req_.send(std::make_shared<spike_model::CacheRequest> (l2_cache_line->getAddr(), CacheRequest::AccessType::WRITEBACK, 0, getClock()->currentCycle(), 0), 1);
            count_wbs_+=1;
            if(trace_)
            {
                logger_.logL2WB(getClock()->currentCycle(), 0, 0, l2_cache_line->getAddr());
            }
        }
        l2_cache_->allocateWithMRUUpdate(*l2_cache_line, phyAddr);


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

            if(!busy_ && !in_flight_reads_.is_full())
            {
                busy_=true;
                //ISSUE EVENT
                issue_access_event_.schedule(sparta::Clock::Cycle(0));
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
            issue_access_event_.schedule(sparta::Clock::Cycle(0));
        }
    }


} // namespace core_example
