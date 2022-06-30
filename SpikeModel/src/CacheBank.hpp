
#ifndef __CacheBank_H__
#define __CacheBank_H__

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"
#include "sparta/resources/Pipeline.hpp"
#include "sparta/resources/Buffer.hpp"
#include "sparta/pairs/SpartaKeyPairs.hpp"
#include "sparta/simulation/State.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"

#include "cache/TreePLRUReplacement.hpp"

#include <unordered_map>

#include "CacheRequest.hpp"
#include "ScratchpadRequest.hpp"
#include "SimpleDL2.hpp"
#include "LogCapable.hpp"
#include "SimulationEntryPoint.hpp"

namespace spike_model
{
    class CacheBank : public sparta::Unit, public LogCapable, public EventVisitor, public SimulationEntryPoint
    {
        using spike_model::EventVisitor::handle; //This prevents the compiler from warning on overloading 

        /*!
         * \class spike_model::CacheBank
         * \brief A cache bank that belongs to a Tile in the architecture.
         *
         * Only one request is issued into the cache per cycle, but up to max_outstanding_misses_ might
         * be in service at the some time. Whether banks are shared or private to the tile is controlled from
         * the FullSystemSimulationFullSystemSimulationEventManager class. The Cache is write-back and write-allocate.
         *
         * This cache might return more than one ack in the same cycle if an access that corresponds to more than one request is serviced. 
         * External arbitration and queueing is necessary to avoid this behavior.
         *
         */
    public:

        /*!
         * \brief Constructor for CacheBank
         * \param node The node that represent the CacheBank and
         * \param p The CacheBank parameter set
         */
        CacheBank(sparta::TreeNode* node, bool always_hit, bool is_writeback, uint16_t miss_latency, uint16_t hit_latency,
                  uint16_t max_outstanding_misses, uint16_t max_in_flight_wbs, bool busy, bool unit_test, uint64_t line_size, uint64_t size_kb,
                  uint64_t associativity, uint32_t bank_and_tile_offset);

        ~CacheBank() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << memory_access_allocator.getNumAllocated()
                          << " MemoryAccessInfo objects allocated/created"
                          << std::endl;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Type Name/Alias Declaration
        ////////////////////////////////////////////////////////////////////////////////


        class MemoryAccessInfo;

        using MemoryAccessInfoPtr = sparta::SpartaSharedPointer<MemoryAccessInfo>;

        // Forward declaration of the Pair Definition class is must as we are friending it.
        // Keep record of memory access information in CacheBank
        class MemoryAccessInfo {
        public:

            MemoryAccessInfo() = delete;

            MemoryAccessInfo(std::shared_ptr<CacheRequest> req) :
                l2_request_(req),
                phyAddrIsReady_(true)
                {
                    address=req->getAddress();
                }

            virtual ~MemoryAccessInfo() {}

            // This ExampleInst pointer will act as our portal to the ExampleInst class
            // and we will use this pointer to query values from functions of ExampleInst class
            std::shared_ptr<CacheRequest> getReq() const { return l2_request_; }

            void setPhyAddrStatus(bool isReady) { phyAddrIsReady_ = isReady; }
            bool getPhyAddrStatus() const { return phyAddrIsReady_; }

            // This is a function which will be added in the addArgs API.
            bool getPhyAddrIsReady() const{
                return phyAddrIsReady_;
            }

            uint64_t getVAdr() const
            {
                return address;
            }

            uint64_t getRAdr() const 
            {
                return address;// | 0x3000; 
            }

        private:

            // load/store instruction pointer
            std::shared_ptr<CacheRequest> l2_request_;

            // Indicate MMU address translation status
            bool phyAddrIsReady_;

            uint64_t address;

        };  // class MemoryAccessInfo

        // allocator for this object type
        sparta::SpartaSharedPointer<MemoryAccessInfo>::SpartaSharedPointerAllocator memory_access_allocator;

        /*!
         * \brief Get a request coming from the input port from the tile and store it for later handling
         * \param req The request to store
         */
        virtual void getAccess_(const std::shared_ptr<Request> & req);

        /*!
        * \brief Issue a request from the queue of pending requests
        */
        void issueAccessInternal_();

        /*!
        * \brief Get the size of the cache bank
        * \return The size
        */
        uint64_t getSize()
        {
            return l2_size_kb_;
        }

        /*!
        * \brief Get the associativity of the cache bank
        * \return The associativity
        */
        uint64_t getAssoc()
        {
            return l2_associativity_;
        }

        /*!
        * \brief Get the line size
        * \return The size of a cache line
        */
        uint64_t getLineSize()
        {
            return l2_line_size_;
        }

        void set_bank_id(uint16_t bank_id)
        {
            bank_id_ = bank_id;
        }

        uint16_t get_bank_id()
        {
            return bank_id_;
        }

        /*!
         * \brief Handles a cache request
         * \param r The event to handle
         */
        void handle(std::shared_ptr<spike_model::CacheRequest> r) override;
        
        /*!
         * \brief Handles a scratchpad request request
         * \param r The event to handle
         */
        void handle(std::shared_ptr<spike_model::ScratchpadRequest> r) override;
            
        /*!
        * \brief Sends an acknowledgement for a serviced request
        * \param req The request to acknowledge
        */
        void sendAckInternal_(const std::shared_ptr<CacheRequest> & req);

        sparta::DataInPort<std::shared_ptr<Request>> in_core_req_
            {&unit_port_set_, "in_tile_req"};

        sparta::DataInPort<std::shared_ptr<CacheRequest>> in_biu_ack_
            {&unit_port_set_, "in_tile_ack"};

        virtual void scheduleIssueAccess(uint64_t ) = 0;
        
    private:

        ////////////////////////////////////////////////////////////////////////////////
        // Input Ports
        ////////////////////////////////////////////////////////////////////////////////



        ////////////////////////////////////////////////////////////////////////////////
        // Output Ports
        ////////////////////////////////////////////////////////////////////////////////
        
        sparta::DataOutPort<std::shared_ptr<Request>> out_core_ack_
            {&unit_port_set_, "out_tile_ack"};

        sparta::DataOutPort<std::shared_ptr<CacheRequest>> out_biu_req_
            {&unit_port_set_, "out_tile_req", false};


        // NOTE:
        // Depending on how many outstanding TLB misses the MMU could handle at the same time
        // This single slot could potentially be extended to a MMU pending miss queue
 
        const bool always_hit_;
        bool writeback_;
        bool cache_busy_ = false;
        // Keep track of the instruction that causes current outstanding cache miss

        // NOTE:
        // Depending on which kind of cache (e.g. blocking vs. non-blocking) is being used
        // This single slot could potentially be extended to a cache pending miss queue


        uint16_t miss_latency_;
        uint16_t hit_latency_;
        uint16_t max_outstanding_misses_;

        bool busy_;
        
        /*!
        * \brief Lookup the cache
        * \param The access for the lookup
        */
        bool cacheLookup_(const MemoryAccessInfoPtr &);
    
        /*
        * \brief Obtain the address of the first byte in the line containing the request
        * \param r The Request
        * \return The address of the line containing the request
        */
        uint64_t calculateLineAddress(std::shared_ptr<CacheRequest> r);

        //An unordered set indexed by the bits of an instruction and containing all its pending (mmu/cache) accesses
        sparta::Counter count_cache_reads_vector_=sparta::Counter(getStatisticSet(), "vector_reads", "Number of vector cache requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_cache_writes_vector_=sparta::Counter(getStatisticSet(), "vector_writes", "Number of vector cache requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_cache_reads_non_vector_=sparta::Counter(getStatisticSet(), "non_vector_reads", "Number of non-vector cache requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_cache_writes_non_vector_=sparta::Counter(getStatisticSet(), "non_vector_writes", "Number of non-vector cache requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_scratchpad_requests_=sparta::Counter(getStatisticSet(), "scratchpad_requests", "Number of scratchpad requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_misses_on_already_pending_=sparta::Counter(getStatisticSet(), "misses_on_already_pending", "Number of misses on addreses that have already been requested", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_vector_misses_=sparta::Counter(getStatisticSet(), "vector_misses", "Number of vector misses", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_non_vector_misses_=sparta::Counter(getStatisticSet(), "non_vector_misses", "Number of non-vector misses", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_stall_=sparta::Counter(getStatisticSet(), "stalls", "Stalls due to full in-flight queue", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_hit_on_store_=sparta::Counter(getStatisticSet(), "hits_on_store", "Number of hits on pending stores", sparta::Counter::COUNT_NORMAL);
        
        sparta::Counter count_non_vector_evicts_non_vector_=sparta::Counter(getStatisticSet(), "non_vector_evicts_non_vector", "Number of cache non vector line eviction caused by a non vector access", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_vector_evicts_non_vector_=sparta::Counter(getStatisticSet(), "vector_evicts_non_vector", "Number of cache non vector line evictions caused by a vector access", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_non_vector_evicts_vector_=sparta::Counter(getStatisticSet(), "non_vector_evicts_vector", "Number of cache vector line evictions caused by a non vector access", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_vector_evicts_vector_=sparta::Counter(getStatisticSet(), "vector_evicts_vector", "Number of cache vector line evictions caused by a vector access", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_vector_evicts_mixed_=sparta::Counter(getStatisticSet(), "vector_evicts_mixed", "Number of cache mixed line evictions caused by a vector access", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_non_vector_evicts_mixed_=sparta::Counter(getStatisticSet(), "non_vector_evicts_mixed", "Number of cache mixed line evictions caused by a non vector access", sparta::Counter::COUNT_NORMAL);
        
        sparta::Counter count_wbs_=sparta::Counter(getStatisticSet(), "writebacks", "Number of writebacks", sparta::Counter::COUNT_NORMAL);
            
        sparta::Counter total_time_spent_by_requests_=sparta::Counter(getStatisticSet(), "total_time_spent_by_requests", "The total time spent by requests", sparta::Counter::COUNT_LATEST);

        sparta::StatisticDef avg_latency_lookup{
            getStatisticSet(), "avg_latency",
            "Average latency",
            getStatisticSet(), "total_time_spent_by_requests/(overall_reads+overall_writes)"
        };
        
        sparta::StatisticDef count_cache_reads_{
            getStatisticSet(), "overall_reads",
            "Number of cache reads (overall)",
            getStatisticSet(), "vector_reads+non_vector_reads"
        };
        
        sparta::StatisticDef count_cache_writes_{
            getStatisticSet(), "overall_writes",
            "Number of cache writes (overall)",
            getStatisticSet(), "vector_writes+non_vector_writes"
        };
        
        sparta::StatisticDef count_cache_misses_{
            getStatisticSet(), "overall_misses",
            "Number of cache misses (overall)",
            getStatisticSet(), "vector_misses+non_vector_misses"
        };
    
        sparta::StatisticDef miss_ratio_{
            getStatisticSet(), "miss_ratio",
            "Miss ratio",
            getStatisticSet(), "overall_misses/(overall_reads+overall_writes)"
        };
        
        sparta::StatisticDef vector_miss_ratio_{
            getStatisticSet(), "vector_miss_ratio",
            "Vector miss ratio",
            getStatisticSet(), "vector_misses/(vector_reads+vector_writes)"
        };
        
        sparta::StatisticDef non_vector_miss_ratio_{
            getStatisticSet(), "non_vector_miss_ratio",
            "Non-vector miss ratio",
            getStatisticSet(), "non_vector_misses/(non_vector_reads+non_vector_writes)"
        };
        
        sparta::StatisticDef count_evictions_{
            getStatisticSet(), "total_evictions",
            "Total evictions",
            getStatisticSet(), "non_vector_evicts_non_vector+vector_evicts_non_vector+non_vector_evicts_vector+vector_evicts_vector+vector_evicts_mixed+non_vector_evicts_mixed"
        };

        class CacheRequestHash
        {
            public:
                size_t operator()(const CacheRequest& h) const
                {
                     return h.getAddress();
                }
        };

        class InFlightMissList
        {
            public:
                InFlightMissList(uint16_t m, uint64_t l):line_size_(l), max_(m){}
                
                bool insert(std::shared_ptr<CacheRequest> req)
                {
                    bool res=false;
                    {
                        res=true;
                        misses_.insert(std::make_pair(getLine(req),req));
                    }
                    return res;
                }
                
                auto equal_range(std::shared_ptr<CacheRequest> req)
                {
                    return misses_.equal_range(getLine(req));
                }

                void erase(std::shared_ptr<CacheRequest> req)
                {
                    misses_.erase(getLine(req));
                }
            
                bool contains(std::shared_ptr<CacheRequest> req)
                {
                    return misses_.find(getLine(req))!=misses_.end();
                }

                bool is_full()
                {
                    return misses_.size()==max_;
                }

            private:
                std::unordered_multimap<uint64_t, std::shared_ptr<CacheRequest>> misses_;
                uint64_t line_size_;
                uint16_t max_;

                uint64_t getLine(std::shared_ptr<CacheRequest> req)
                {
                    return (req->getAddress()/line_size_)*line_size_;
                }
        };

        InFlightMissList in_flight_misses_;
        uint16_t max_in_flight_wbs;
        uint16_t num_in_flight_wbs;
        std::shared_ptr<CacheRequest> pending_wb;

        std::list<std::shared_ptr<CacheRequest>> pending_fetch_requests_;
        std::list<std::shared_ptr<CacheRequest>> pending_load_requests_;
        std::list<std::shared_ptr<CacheRequest>> pending_store_requests_;
        std::list<std::shared_ptr<ScratchpadRequest>> pending_scratchpad_requests_;
        
        uint64_t l2_size_kb_;
        uint64_t l2_associativity_;
        uint64_t l2_line_size_;
        uint16_t bank_id_;

        uint32_t bank_and_tile_offset_;

        long long d;
   
        virtual void logCacheRequest(std::shared_ptr<CacheRequest> r)=0;

    protected:
        std::map<uint64_t, uint64_t> eviction_times_;
        bool unit_test;
        // Using the same handling policies as the L1 Data Cache
        using DL1Handle = SimpleDL2::Handle;
        DL1Handle l2_cache_;
        
        /*!
         * \brief Enqueue an event to the tile
         * \param The event to submit 
         */
        virtual void putEvent(const std::shared_ptr<Event> & ) override;
        
        /*!
        * \brief Handle the cache access for a request
        * \param mem_access_info_ptr The access to handle
        */
        virtual bool handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr);

        /*!
        * \brief Update the replacement info for an address
        * \param The address to update
        */
        virtual void reloadCache_(uint64_t, uint16_t, CacheRequest::AccessType, bool is_vector);
    };



} // namespace spike_model

#endif

