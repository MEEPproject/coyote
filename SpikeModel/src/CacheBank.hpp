
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

#include "SimpleDL1.hpp"

#include "LogCapable.hpp"

namespace spike_model
{
    class CacheBank : public sparta::Unit, public LogCapable, public spike_model::EventVisitor
    {
        using spike_model::EventVisitor::handle; //This prevents the compiler from warning on overloading 

        /*!
         * \class spike_model::CacheBank
         * \brief A cache bank that belongs to a Tile in the architecture.
         *
         * Only one request is issued into the cache per cycle, but up to max_outstanding_misses_ might
         * be in service at the sime time. Whether banks are shared or private to the tile is controlled from
         * the EventManager class. The Cache is write-back and write-allocate.
         */
    public:
        /*!
         * \class CacheBankParameterSet
         * \brief Parameters for CacheBank model
         */
        class CacheBankParameterSet : public sparta::ParameterSet
        {
        public:
            //! Constructor for CacheBankParameterSet
            CacheBankParameterSet(sparta::TreeNode* n):
                sparta::ParameterSet(n)
            {
            }

            PARAMETER(uint64_t, line_size, 128, "DL1 line size (power of 2)")
            PARAMETER(uint64_t, size_kb, 2048, "Size of DL1 in KB (power of 2)")
            PARAMETER(uint64_t, associativity, 8, "DL1 associativity (power of 2)")
            PARAMETER(uint64_t, bank_and_tile_offset, 1, "The stride for banks and tiles. It is log2'd to get the bits that need to be shifted to identify the set")
            PARAMETER(bool, always_hit, false, "DL1 will always hit")
            // Parameters for event scheduling
            PARAMETER(uint16_t, miss_latency, 10, "Cache miss latency")
            PARAMETER(uint16_t, hit_latency, 10, "Cache hit latency")
            PARAMETER(uint16_t, max_outstanding_misses, 8, "Maximum misses in flight to the next level")
        };

        /*!
         * \brief Constructor for CacheBank
         * \param node The node that represent the CacheBank and
         * \param p The CacheBank parameter set
         */
        CacheBank(sparta::TreeNode* node, const CacheBankParameterSet* p);

        ~CacheBank() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << memory_access_allocator.getNumAllocated()
                          << " MemoryAccessInfo objects allocated/created"
                          << std::endl;
        }

        //! name of this resource.
        static const char name[];


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
        void getAccess_(const std::shared_ptr<Request> & req);

        /*!
        * \brief Issue a request from the queue of pending requests
        */
        void issueAccess_();

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
            
    private:

        ////////////////////////////////////////////////////////////////////////////////
        // Output Ports
        ////////////////////////////////////////////////////////////////////////////////

        sparta::DataInPort<std::shared_ptr<Request>> in_core_req_
            {&unit_port_set_, "in_tile_req"};

        sparta::DataInPort<std::shared_ptr<CacheRequest>> in_biu_ack_
            {&unit_port_set_, "in_tile_ack"};


        ////////////////////////////////////////////////////////////////////////////////
        // Output Ports
        ////////////////////////////////////////////////////////////////////////////////
        
        sparta::DataOutPort<std::shared_ptr<Request>> out_core_ack_
            {&unit_port_set_, "out_tile_ack"};

        sparta::DataOutPort<std::shared_ptr<CacheRequest>> out_biu_req_
            {&unit_port_set_, "out_tile_req", false};


        sparta::UniqueEvent<> issue_access_event_ 
            {&unit_event_set_, "issue_access_", CREATE_SPARTA_HANDLER(CacheBank, issueAccess_)};

        // NOTE:
        // Depending on how many outstanding TLB misses the MMU could handle at the same time
        // This single slot could potentially be extended to a mmu pending miss queue


        // L1 Data Cache
        using DL1Handle = SimpleDL1::Handle;
        DL1Handle l2_cache_;
        const bool always_hit_;
        bool cache_busy_ = false;
        // Keep track of the instruction that causes current outstanding cache miss

        // NOTE:
        // Depending on which kind of cache (e.g. blocking vs. non-blocking) is being used
        // This single slot could potentially be extended to a cache pending miss queue


        uint16_t miss_latency_;
        uint16_t hit_latency_;
        uint16_t max_outstanding_misses_;

        bool busy_;

        ////////////////////////////////////////////////////////////////////////////////
        // Callbacks
        ////////////////////////////////////////////////////////////////////////////////

        /*!
        * \brief Sends an acknoledgement for a serviced request
        * \param req The request to acknowledge
        */
        void sendAck_(const std::shared_ptr<CacheRequest> & req);



        ////////////////////////////////////////////////////////////////////////////////
        // Regular Function/Subroutine Call
        ////////////////////////////////////////////////////////////////////////////////

        /*!
        * \brief Handle the cache access for a request
        * \param mem_access_info_ptr The access to handle
        */
        bool handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr);

        /*!
        * \brief Lookup the cache
        * \param The access for the lookup
        */
        bool cacheLookup_(const MemoryAccessInfoPtr &);

        /*!
        * \brief Update the replacement info for an address
        * \param The address to update
        */
        void reloadCache_(uint64_t);
    
        /*
        * \brief Obtain the address of the first byte in the line containing the request
        * \param r The Request
        * \return The address of the line containing the request
        */
        uint64_t calculateLineAddress(std::shared_ptr<CacheRequest> r);

        //An unordered set indexed by the bits of an instruction and containing all its pending (mmu/cache) accesses
        sparta::Counter count_cache_requests_=sparta::Counter(getStatisticSet(), "cache_requests", "Number of cache requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_scratchpad_requests_=sparta::Counter(getStatisticSet(), "scratchpad_requests", "Number of scratchpad requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_misses_on_already_pending_=sparta::Counter(getStatisticSet(), "misses_on_already_pending", "Number of misses on addreses that have already been requested", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_cache_misses_=sparta::Counter(getStatisticSet(), "cache_misses", "Number of cache misses", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_stall_=sparta::Counter(getStatisticSet(), "stalls", "Stalls due to full in-flight queue", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_hit_on_store_=sparta::Counter(getStatisticSet(), "hits_on_store", "Number of hits on pending stores", sparta::Counter::COUNT_NORMAL);
        
        sparta::Counter count_wbs_=sparta::Counter(getStatisticSet(), "writebacks", "Number of writebacks", sparta::Counter::COUNT_NORMAL);
        
        sparta::StatisticDef miss_ratio_{
            getStatisticSet(), "miss_ratio",
            "Miss ratio",
            getStatisticSet(), "cache_misses/cache_requests"
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

        InFlightMissList in_flight_reads_;

        std::list<std::shared_ptr<CacheRequest>> pending_fetch_requests_;
        std::list<std::shared_ptr<CacheRequest>> pending_load_requests_;
        std::list<std::shared_ptr<CacheRequest>> pending_store_requests_;
        std::list<std::shared_ptr<ScratchpadRequest>> pending_scratchpad_requests_;

        std::map<uint64_t, uint64_t> eviction_times_;

        uint64_t l2_size_kb_;
        uint64_t l2_associativity_;
        uint64_t l2_line_size_;

        uint8_t bank_and_tile_offset_;

        long long d;
    };



} // namespace spike_model

#endif

