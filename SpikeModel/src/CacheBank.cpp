
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
        pending_requests_(),
        l2_size_kb_ (p->size_kb),
        l2_associativity_ (p->associativity),
        l2_line_size_ (p->line_size)
    {

        in_core_req_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(CacheBank, getAccess_, std::shared_ptr<Request>));

        in_biu_ack_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(CacheBank, sendAck_, std::shared_ptr<Request>));


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
    void CacheBank::sendAck_(const std::shared_ptr<Request> & req)
    {
        bool was_stalled=in_flight_reads_.is_full();

        if(req->getType()==Request::AccessType::LOAD || req->getType()==Request::AccessType::FETCH)
        {
            reloadCache_(req->getLineAddress() | 0x3000); //THIS IS A WORKAROUND TO GET RID OF MEMACCESSPTRS SHOULD ENCAPSULATE THE CALCULATION OF THE REAL ADDRESS
            auto range_misses=in_flight_reads_.equal_range(req);

            sparta_assert(range_misses.first != range_misses.second, "Got an ack for an unrequested miss\n");

            while(range_misses.first != range_misses.second)
            {
                out_core_ack_.send(range_misses.first->second);
                range_misses.first++;
            }

            in_flight_reads_.erase(req);
        
        }
        else //STORE-WRITEBACK JUST NOTIFY THIS REQUEST, AS THERE ISN'T ANY OTHER ASSOCIATED
        {
            out_core_ack_.send(req);
        }

        if(pending_requests_.size()>0)
        {
            //ISSUE EVENT
            if(was_stalled)
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
        count_requests_+=1;


        bool hit_on_store=false;
        if(req->getType()!=Request::AccessType::LOAD)
        {
            //CHECK FOR HITS ON PENDING WRITEBACK. STORES ARE NOT CHECKED. YOU NEED TO LOAD A WHOLE LINE, NOT JUST A WORD.
            for (std::list<std::shared_ptr<Request>>::reverse_iterator rit=pending_requests_.rbegin(); rit!=pending_requests_.rend() && !hit_on_store; ++rit)
            {
                hit_on_store=((*rit)->getType()==Request::AccessType::WRITEBACK && (*rit)->getLineAddress()==req->getLineAddress());
            }
        }

        if(hit_on_store)
        {
            //AUTO HIT
            out_core_ack_.send(req,1);
            count_hit_on_store_++;
        }
        else
        {
            pending_requests_.push_back(req);
            if(!busy_ && !in_flight_reads_.is_full())
            {
                busy_=true;
                //ISSUE EVENT
                issue_access_event_.schedule(sparta::Clock::Cycle(0));
            }
        }
    }

    void CacheBank::issueAccess_()
    {
        MemoryAccessInfoPtr m = sparta::allocate_sparta_shared_pointer<MemoryAccessInfo>(memory_access_allocator, pending_requests_.front());
        pending_requests_.pop_front();
        handleCacheLookupReq_(m);

        bool stall=false;
        if(in_flight_reads_.is_full())
        {
            stall=true;
            count_stall_++;
        }


        if(!stall && pending_requests_.size()>0) //IF THERE ARE PENDING REQUESTS, TRY TO SCHEDULE NEXT ISSUE
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

                if(mem_access_info_ptr->getReq()->getType()==Request::AccessType::LOAD || mem_access_info_ptr->getReq()->getType()==Request::AccessType::FETCH)
                {
                    already_pending=in_flight_reads_.contains(mem_access_info_ptr->getReq());
                    in_flight_reads_.insert(mem_access_info_ptr->getReq());
                }


                if(!already_pending)
                {
                    out_biu_req_.send(mem_access_info_ptr->getReq(), sparta::Clock::Cycle(miss_latency_));
                }
                else
                {
                    count_misses_on_already_pending_++;
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
        l2_cache_->allocateWithMRUUpdate(*l2_cache_line, phyAddr);


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "Cache reload complete!";
        }
    }


} // namespace core_example
