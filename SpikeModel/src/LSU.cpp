
#include "sparta/utils/SpartaAssert.hpp"
#include "LSU.hpp"
#include <chrono>

namespace spike_model
{
    const char LSU::name[] = "lsu";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    LSU::LSU(sparta::TreeNode *node, const LSUParameterSet *p) :
        sparta::Unit(node),
        memory_access_allocator(2000, 1000),   // 50 and 30 are arbitrary numbers here.  It can be tuned to an exact value.
        load_store_info_allocator(2000, 1000),
        ldst_inst_queue_("lsu_inst_queue", p->ldst_inst_queue_size, getClock()),
        ldst_inst_queue_size_(p->ldst_inst_queue_size),

        tlb_always_hit_(p->tlb_always_hit),
        dl1_always_hit_(p->dl1_always_hit),

        issue_latency_(p->issue_latency),
        mmu_latency_(p->mmu_latency),
        cache_latency_(p->cache_latency),
        complete_latency_(p->complete_latency)
    {
        // Pipeline collection config
        ldst_pipeline_.enableCollection(node);
        ldst_inst_queue_.enableCollection(node);



        in_biu_ack_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(LSU, getAckFromBIU_, MemoryAccessInfoPtr));

        // Pipeline events config
        ldst_pipeline_.performOwnUpdates();
//        ldst_pipeline_.registerHandlerAtStage(static_cast<uint32_t>(PipelineStage::MMU_LOOKUP),
//                                                CREATE_SPARTA_HANDLER(LSU, handleMMULookupReq_));



        // Event precedence setup
        uev_cache_drive_biu_port_ >> uev_mmu_drive_biu_port_;

        // NOTE:
        // To resolve the race condition when:
        // Both cache and MMU try to drive the single BIU port at the same cycle
        // Here we give cache the higher priority

        // DL1 cache config
        const uint64_t dl1_line_size = p->dl1_line_size;
        const uint64_t dl1_size_kb = p->dl1_size_kb;
        const uint64_t dl1_associativity = p->dl1_associativity;
        std::unique_ptr<sparta::cache::ReplacementIF> repl(new sparta::cache::TreePLRUReplacement
                                                         (dl1_associativity));
        dl1_cache_.reset(new SimpleDL1( getContainer(), dl1_size_kb, dl1_line_size, *repl ));

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "LSU construct: #" << node->getGroupIdx();
        }
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Callbacks
    ////////////////////////////////////////////////////////////////////////////////

    // Receive MSS access acknowledge from Bus Interface Unit
    void LSU::getAckFromBIU_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        auto range_mmu=pending_insts_mmu.equal_range(*mem_access_info_ptr);
        bool found=false;
        while(range_mmu.first != range_mmu.second && !found)
        {
            found=range_mmu.first->getVAdr()==mem_access_info_ptr->getVAdr();
            if(found)
            {
                pending_insts_mmu.erase(range_mmu.first);
            }
            else
            {
                range_mmu.first++;
            }
        }
        if (found) {
            rehandleMMULookupReq_(mem_access_info_ptr);
            
            range_mmu=pending_insts_mmu.equal_range(*mem_access_info_ptr);
            if(range_mmu.first==range_mmu.second && range_mmu.first==range_mmu.second)
            {
                //All pending accesses have been satisfied. Reissue.
                MemoryInstruction m=mem_access_info_ptr->getInst();
                issueInst_(m, true);
            }
        }
        else
        {
            auto range_cache=pending_insts_cache.equal_range(*mem_access_info_ptr);
            found=false;
            while(range_cache.first != range_cache.second && !found)
            {
                found=range_cache.first->getVAdr()==mem_access_info_ptr->getVAdr();
                if(found)
                {
                    pending_insts_cache.erase(range_cache.first);
                }
                else
                {
                    range_cache.first++;
                }
            }
            if (found) {
                rehandleCacheLookupReq_(mem_access_info_ptr);
                
                range_cache=pending_insts_cache.equal_range(*mem_access_info_ptr);
                if(range_cache.first==range_cache.second && range_cache.first==range_cache.second)
                {
                    //All pending accesses have been satified. Reissue.
                    MemoryInstruction m=mem_access_info_ptr->getInst();
                    issueInst_(m, true);
                }
            }
            else 
            {
                sparta_assert(false, "Unexpected BIU Ack event occurs!");
            }
        }
        
    }


    // Issue inst. Returns true if it hits
    bool LSU::issueInst_(MemoryInstruction & insn, bool isReIssue)
    {
        bool hit=true;
            
        /*if(insn.getAccesses().size()!=1)
        {
            std::cout << "Hey, instruction " << insn << " generates " << insn.getAccesses().size() << " accesses\n";
        }*/
        if(!isReIssue)
        {
            count_cache_accesses_+=insn.getAccesses().size();
        }
 
        for(memory_access_t a: insn.getAccesses())
        {
            MemoryAccessInfoPtr m = sparta::allocate_sparta_shared_pointer<MemoryAccessInfo>(memory_access_allocator,
                                                                                                    insn,
                                                                                                    a);
            bool hit_cache=false;
            bool hit_tlb=handleMMULookupReq_(m);
            if(hit_tlb)
            {
                hit_cache=handleCacheLookupReq_(m);
            }
            hit&=hit_cache;
        }
        if(isReIssue && hit)
        {
           dispatcher_->markRegisterAsAvailable(insn.getRd());
        }

        return hit;
    }

    // Handle MMU access request
    bool LSU::handleMMULookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {

        bool isAlreadyHIT = (mem_access_info_ptr->getMMUState() == MemoryAccessInfo::MMUState::HIT);
        bool MMUBypass = isAlreadyHIT;

        if (MMUBypass) {

            if (SPARTA_EXPECT_FALSE(info_logger_.observed())) {
                info_logger_ << "MMU Lookup is skipped (TLB is already hit)!";
            }

            return true;
        }

        //auto t1 = std::chrono::high_resolution_clock::now();
    
        // Access TLB, and check TLB hit or miss
        bool TLB_HIT = MMULookup_(mem_access_info_ptr);

        //auto t2 = std::chrono::high_resolution_clock::now();

        //auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

        //d2+=duration;        

        //std::cout << "MMU lookup took " << duration << " ns (" << d2 << " total)\n";

        if (TLB_HIT) {
            // Update memory access info
            mem_access_info_ptr->setMMUState(MemoryAccessInfo::MMUState::HIT);
            // Update physical address status
            mem_access_info_ptr->setPhyAddrStatus(true);
        }
        else {
            // Update memory access info
            mem_access_info_ptr->setMMUState(MemoryAccessInfo::MMUState::MISS);

            if (mmu_busy_ == false) {
                // MMU is busy, no more TLB MISS can be handled, RESET is required on finish
                //mmu_busy_ = true;
                // Keep record of the current TLB MISS instruction

                pending_insts_mmu.insert(*mem_access_info_ptr);



                // NOTE:
                // mmu_busy_ RESET could be done:
                // as early as port-driven event for this miss finish, and
                // as late as TLB reload event for this miss finish.

                // Schedule port-driven event in BIU
//                uev_mmu_drive_biu_port_.schedule(sparta::Clock::Cycle(0));

                out_biu_req_.send(mem_access_info_ptr, sparta::Clock::Cycle(latency_tlb_miss_));

                // NOTE:
                // The race between simultaneous MMU and cache requests is resolved by
                // specifying precedence between these two competing events


                if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
                    info_logger_ << "MMU is trying to drive BIU request port!";
                }
            }
            else {

                if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
                    info_logger_
                        << "MMU miss cannot be served right now due to another outstanding one!";
                }
            }

            // NEW: Invalidate pipeline stage
//            ldst_pipeline_.invalidateStage(static_cast<uint32_t>(PipelineStage::MMU_LOOKUP));
        }
        return TLB_HIT;
    }

    // Drive BIU request port from MMU
    void LSU::driveBIUPortFromMMU_()
    {
        /*bool succeed = false;

        // Check if DataOutPort is available
        if (!out_biu_req_.isDriven()) {
            sparta_assert(mmu_pending_inst_ptr_ != nullptr,
                "Attempt to drive BIU port when no outstanding TLB miss exists!");

            // Port is available, drive port immediately, and send request out
            out_biu_req_.send(mmu_pending_inst_ptr_);

            succeed = true;
        }
        else {
            // Port is being driven by another source, wait for one cycle and check again
            uev_mmu_drive_biu_port_.schedule(sparta::Clock::Cycle(1));
        }


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            if (succeed) {
                info_logger_ << "MMU is driving the BIU request port!";
            }
            else {
                info_logger_ << "MMU is waiting to drive the BIU request port!";
            }
        }
        */
    }

    // Handle cache access request
    bool LSU::handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        const bool phyAddrIsReady =
            mem_access_info_ptr->getPhyAddrStatus();
        const bool isAlreadyHIT =
            (mem_access_info_ptr->getCacheState() == MemoryAccessInfo::CacheState::HIT);
        const bool cacheBypass = isAlreadyHIT || !phyAddrIsReady;
        
        if (cacheBypass) {

            if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
                if (isAlreadyHIT) {
                    info_logger_ << "Cache Lookup is skipped (Cache already hit)!";
                }
                else if (!phyAddrIsReady) {
                    info_logger_ << "Cache Lookup is skipped (Physical address not ready)!";
                }
                else {
                    sparta_assert(false, "Cache access is bypassed without a valid reason!");
                }
            }

            return true;
        }
        

        //auto t1 = std::chrono::high_resolution_clock::now();
        // Access cache, and check cache hit or miss
        const bool CACHE_HIT = cacheLookup_(mem_access_info_ptr);
  //      auto t2 = std::chrono::high_resolution_clock::now();

    //    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

      //  d+=duration;

        //std::cout << "Cache lookup took " << duration << " ns (" << d <<  " total)\n";


        if (CACHE_HIT) {
            // Update memory access info
            mem_access_info_ptr->setCacheState(MemoryAccessInfo::CacheState::HIT);
        }
        else {
            count_cache_misses_++;
            // Update memory access info
            mem_access_info_ptr->setCacheState(MemoryAccessInfo::CacheState::MISS);

            if (cache_busy_ == false) {
                // Cache is now busy, no more CACHE MISS can be handled, RESET required on finish
                //cache_busy_ = true;
                // Keep record of the current CACHE MISS instruction
                
                pending_insts_cache.insert(*mem_access_info_ptr);

                // NOTE:
                // cache_busy_ RESET could be done:
                // as early as port-driven event for this miss finish, and
                // as late as cache reload event for this miss finish.

                // Schedule port-driven event in BIU
//                uev_cache_drive_biu_port_.schedule(sparta::Clock::Cycle(0));
                out_biu_req_.send(mem_access_info_ptr, sparta::Clock::Cycle(latency_cache_miss_));

                // NOTE:
                // The race between simultaneous MMU and cache requests is resolved by
                // specifying precedence between these two competing events


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

    // Drive BIU request port from cache
    void LSU::driveBIUPortFromCache_()
    {
        /*
        bool succeed = false;

        // Check if DataOutPort is available
        if (!out_biu_req_.isDriven()) {
            sparta_assert(cache_pending_inst_ptr_ != nullptr,
                "Attempt to drive BIU port when no outstanding cache miss exists!");

            // Port is available, drive the port immediately, and send request out
            out_biu_req_.send(cache_pending_inst_ptr_);

            succeed = true;
        }
        else {
            // Port is being driven by another source, wait for one cycle and check again
            uev_cache_drive_biu_port_.schedule(sparta::Clock::Cycle(1));
        }


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            if (succeed) {
                info_logger_ << "Cache is driving the BIU request port!";
            }
            else {
                info_logger_ << "Cache is waiting to drive the BIU request port!";
            }
        }
        */
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Regular Function/Subroutine Call
    ////////////////////////////////////////////////////////////////////////////////

    // Append new load/store instruction into issue queue
/*    void LSU::appendIssueQueue_(const LoadStoreInstInfoPtr & inst_info_ptr)
    {
        sparta_assert(ldst_inst_queue_.size() <= ldst_inst_queue_size_,
                        "Appending issue queue causes overflows!");

        // Always append newly dispatched instructions to the back of issue queue
        ldst_inst_queue_.push_back(inst_info_ptr);


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "Append new load/store instruction to issue queue!";
        }
    }

    // Pop completed load/store instruction out of issue queue
    void LSU::popIssueQueue_(const std::shared_ptr<MemoryInstruction> & inst_ptr)
    {
        // Look for the instruction to be completed, and remove it from issue queue
        for (auto iter = ldst_inst_queue_.begin(); iter != ldst_inst_queue_.end(); iter++) {
            if ((*iter)->getInstPtr() == inst_ptr) {
                ldst_inst_queue_.erase(iter);

                return;
            }
        }

        sparta_assert(false, "Attempt to complete instruction no longer exiting in issue queue!");
    }
*/

    // Check for ready to issue instructions
    bool LSU::isReadyToIssueInsts_() const
    {
        bool isReady = false;

        // Check if there is at least one ready-to-issue instruction in issue queue
        for (auto const &inst_info_ptr : ldst_inst_queue_) {
            if (inst_info_ptr->isReady()) {
                isReady = true;

                break;
            }
        }


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            if (isReady) {
                info_logger_ << "At least one more instruction is ready to be issued!";
            }
            else {
                info_logger_ << "No more instruction is ready to be issued!";
            }
        }

        return isReady;
    }


    // Access MMU/TLB
    bool LSU::MMULookup_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        uint64_t vaddr = mem_access_info_ptr->getVAdr();

        bool tlb_hit = false;

        // C++ comma operator: assign tlb_hit first, then evaluate it. Just For Fun
        if (tlb_hit = tlb_always_hit_, tlb_hit) {
        }
        else {
        //auto t1 = std::chrono::high_resolution_clock::now();
            auto tlb_entry = tlb_cache_->peekLine(vaddr);
            tlb_hit = (tlb_entry != nullptr) && tlb_entry->isValid();

        //auto t2 = std::chrono::high_resolution_clock::now();

        //auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

        //d+=duration;

        //std::cout << "MMU peek took " << duration << " ns (" << d <<  " total)\n";
            // Update MRU replacement state if TLB HIT
            if (tlb_hit) {
        //auto t1 = std::chrono::high_resolution_clock::now();
                tlb_cache_->touch(*tlb_entry);
        //auto t2 = std::chrono::high_resolution_clock::now();

        //auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();

        //d2+=duration;

        //std::cout << "MMU touch took " << duration << " ns (" << d2 <<  " total)\n";
            }
        }


        if (SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            if (tlb_always_hit_) {
                info_logger_ << "TLB HIT all the time: vaddr=0x" << std::hex << vaddr;
            }
            else if (tlb_hit) {
                info_logger_ << "TLB HIT: vaddr=0x" << std::hex << vaddr;
            }
            else {
                info_logger_ << "TLB MISS: vaddr=0x" << std::hex << vaddr;
            }
        }

        return tlb_hit;
    }

    // Re-handle outstanding MMU access request
    void LSU::rehandleMMULookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        // MMU is no longer busy any more
        mmu_busy_ = false;
//        mmu_pending_inst_ptr_.reset();

        // NOTE:
        // MMU may not have to wait until MSS Ack comes back
        // MMU could be ready to service new TLB MISS once previous request has been sent
        // However, that means MMU has to keep record of a list of pending instructions


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "BIU Ack for an outstanding MMU miss is received!";
        }

        // Reload TLB entry
        reloadTLB_(mem_access_info_ptr->getVAdr());

        // Update issue priority & Schedule an instruction (re-)issue event
//        updateIssuePriorityAfterTLBReload_(inst_ptr);
//        uev_issue_inst_.schedule(sparta::Clock::Cycle(0));


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "MMU rehandling event is scheduled!";
        }
    }

    // Reload TLB entry
    void LSU::reloadTLB_(uint64_t vaddr)
    {
        auto tlb_entry = &tlb_cache_->getLineForReplacementWithInvalidCheck(vaddr);
        tlb_cache_->allocateWithMRUUpdate(*tlb_entry, vaddr);


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "TLB reload complete!";
        }
    }

    // Access Cache
    bool LSU::cacheLookup_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        uint64_t phyAddr = mem_access_info_ptr->getRAdr();

        bool cache_hit = false;

        if (dl1_always_hit_) {
            cache_hit = true;
        }
        else {
            auto cache_line = dl1_cache_->peekLine(phyAddr);
            cache_hit = (cache_line != nullptr) && cache_line->isValid();
            
            if (cache_hit) {
                dl1_cache_->touchMRU(*cache_line);
            }
        }


        if (SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            if (dl1_always_hit_) {
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

    // Re-handle outstanding cache access request
    void LSU::rehandleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr)
    {
        // Cache is no longer busy any more
        cache_busy_ = false;
//        cache_pending_inst_ptr_.reset();

        // NOTE:
        // Depending on cache is blocking or not,
        // It may not have to wait until MMS Ack returns.
        // However, that means cache has to keep record of a list of pending instructions

        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "BIU Ack for an outstanding cache miss is received!";
        }

        // Reload cache line
        reloadCache_(mem_access_info_ptr->getRAdr());

        // Update issue priority & Schedule an instruction (re-)issue event
//        updateIssuePriorityAfterCacheReload_(inst_ptr);
//        uev_issue_inst_.schedule(sparta::Clock::Cycle(0));


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "Cache rehandling event is scheduled!";
        }
    }
    // Reload cache line
    void LSU::reloadCache_(uint64_t phyAddr)
    {
        auto dl1_cache_line = &dl1_cache_->getLineForReplacementWithInvalidCheck(phyAddr);
        dl1_cache_->allocateWithMRUUpdate(*dl1_cache_line, phyAddr);


        if(SPARTA_EXPECT_FALSE(info_logger_.observed())) {
            info_logger_ << "Cache reload complete!";
        }
    }


} // namespace core_example
