
#include "sparta/utils/SpartaAssert.hpp"
#include "L2Cache.hpp"
#include <chrono>

namespace spike_model
{
    const char L2Cache::name[] = "l2";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    L2Cache::L2Cache(sparta::TreeNode *node, const L2CacheParameterSet *p) :
        sparta::Unit(node),
        memory_access_allocator(2000, 1000),
        always_hit_(p->always_hit),
        miss_latency_(p->miss_latency),
        hit_latency_(p->hit_latency),
        in_flight_reads_(p->line_size),
        pending_requests_(p->line_size)
    {

        in_core_req_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(L2Cache, issueAccess_, L2Request));

        in_biu_ack_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(L2Cache, sendAck_, MemoryAccessInfoPtr));

        // NOTE:
        // To resolve the race condition when:
        // Both cache and MMU try to drive the single BIU port at the same cycle
        // Here we give cache the higher priority

        // DL1 cache config
        const uint64_t l2_line_size = p->line_size;
        const uint64_t l2_size_kb = p->size_kb;
        const uint64_t l2_associativity = p->associativity;
        std::unique_ptr<sparta::cache::ReplacementIF> repl(new sparta::cache::TreePLRUReplacement
                                                         (l2_associativity));
        l2_cache_.reset(new SimpleDL1( getContainer(), l2_size_kb, l2_line_size, *repl ));

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "L2Cache construct: #" << node->getGroupIdx();
        }
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive MSS access acknowledge from Bus Interface Unit
    void L2Cache::sendAck_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        reloadCache_(mem_access_info_ptr->getRAdr());
        if(mem_access_info_ptr->getReq().getType()!=L2Request::AccessType::STORE)
        {
            auto range_misses=in_flight_reads_.equal_range(mem_access_info_ptr->getReq());

            sparta_assert(range_misses.first != range_misses.second, "Got an ack for an unrequested miss\n");

            while(range_misses.first != range_misses.second)
            {
                out_core_ack_.send(range_misses.first->second);
                range_misses.first++;
            }

            in_flight_reads_.erase(mem_access_info_ptr->getReq());
        }
    }   


    // Issue inst. Returns true if it hits
    void L2Cache::issueAccess_(const L2Request & req)
    {
        count_requests_+=1;

        MemoryAccessInfoPtr m = sparta::allocate_sparta_shared_pointer<MemoryAccessInfo>(memory_access_allocator, req);
        handleCacheLookupReq_(m);
    }

    // Handle cache access request
    bool L2Cache::handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        // Access cache, and check cache hit or miss
        const bool CACHE_HIT = cacheLookup_(mem_access_info_ptr);

        if (CACHE_HIT) {
            // Update memory access info
            out_core_ack_.send(mem_access_info_ptr->getReq(), hit_latency_);
        }
        else {
            count_cache_misses_++;
            // Update memory access info

            if (cache_busy_ == false) {
                // Cache is now busy, no more CACHE MISS can be handled, RESET required on finish
                //cache_busy_ = true;
                // Keep record of the current CACHE MISS instruction
                
                // NOTE:
                // cache_busy_ RESET could be done:
                // as early as port-driven event for this miss finish, and
                // as late as cache reload event for this miss finish.

                bool already_pending=false;

                if(mem_access_info_ptr->getReq().getType()!=L2Request::AccessType::STORE)
                {
                    already_pending=in_flight_reads_.contains(mem_access_info_ptr->getReq());
                    in_flight_reads_.insert(mem_access_info_ptr->getReq());
                }


                if(!already_pending)
                {
                    out_biu_req_.send(mem_access_info_ptr, sparta::Clock::Cycle(miss_latency_));
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
    bool L2Cache::cacheLookup_(const MemoryAccessInfoPtr & mem_access_info_ptr)
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
    void L2Cache::reloadCache_(uint64_t phyAddr)
    {
        auto l2_cache_line = &l2_cache_->getLineForReplacementWithInvalidCheck(phyAddr);
        l2_cache_->allocateWithMRUUpdate(*l2_cache_line, phyAddr);


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "Cache reload complete!";
        }
    }


} // namespace core_example
