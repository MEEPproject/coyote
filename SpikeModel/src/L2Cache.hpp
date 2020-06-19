
#ifndef __L2Cache_H__
#define __L2Cache_H__

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

#include "L2Request.hpp"

#include "SimpleDL1.hpp"

namespace spike_model
{
    class L2Cache : public sparta::Unit
    {
    public:
        /*!
         * \class L2CacheParameterSet
         * \brief Parameters for L2Cache model
         */
        class L2CacheParameterSet : public sparta::ParameterSet
        {
        public:
            //! Constructor for L2CacheParameterSet
            L2CacheParameterSet(sparta::TreeNode* n):
                sparta::ParameterSet(n)
            {
            }

            PARAMETER(uint64_t, line_size, 128, "DL1 line size (power of 2)")
            PARAMETER(uint64_t, size_kb, 2048, "Size of DL1 in KB (power of 2)")
            PARAMETER(uint64_t, associativity, 8, "DL1 associativity (power of 2)")
            PARAMETER(bool, always_hit, false, "DL1 will always hit")
            // Parameters for event scheduling
            PARAMETER(uint32_t, miss_latency, 100, "Cache miss latency")
            PARAMETER(uint32_t, hit_latency, 10, "Cache hit latency")
        };

        /*!
         * \brief Constructor for L2Cache
         * \note  node parameter is the node that represent the L2Cache and
         *        p is the L2Cache parameter set
         */
        L2Cache(sparta::TreeNode* node, const L2CacheParameterSet* p);

        ~L2Cache() {
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
        // Keep record of memory access information in L2Cache
        class MemoryAccessInfo {
        public:

            MemoryAccessInfo() = delete;

            MemoryAccessInfo(L2Request req) :
                l2_request_(req),
                phyAddrIsReady_(true),
                // Construct the State object here
                address(req.getAddress()){}

            virtual ~MemoryAccessInfo() {}

            // This ExampleInst pointer will act as our portal to the ExampleInst class
            // and we will use this pointer to query values from functions of ExampleInst class
            L2Request getReq() const { return l2_request_; }

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
                return address | 0x3000; 
            }

        private:

            // load/store instruction pointer
            L2Request l2_request_;

            // Indicate MMU address translation status
            bool phyAddrIsReady_;

            uint64_t address;

        };  // class MemoryAccessInfo

        // allocator for this object type
        sparta::SpartaSharedPointer<MemoryAccessInfo>::SpartaSharedPointerAllocator memory_access_allocator;

        // Issue/Re-issue ready instructions in the issue queue
        void issueAccess_(const L2Request & req);

    private:

        ////////////////////////////////////////////////////////////////////////////////
        // Output Ports
        ////////////////////////////////////////////////////////////////////////////////

        sparta::DataInPort<L2Request> in_core_req_
            {&unit_port_set_, "in_noc_req"};

        sparta::DataInPort<MemoryAccessInfoPtr> in_biu_ack_
            {&unit_port_set_, "in_biu_ack"};


        ////////////////////////////////////////////////////////////////////////////////
        // Output Ports
        ////////////////////////////////////////////////////////////////////////////////
        
        sparta::DataOutPort<L2Request> out_core_ack_
            {&unit_port_set_, "out_noc_ack"};

        sparta::DataOutPort<MemoryAccessInfoPtr> out_biu_req_
            {&unit_port_set_, "out_biu_req"};



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


        unsigned miss_latency_;
        unsigned hit_latency_;

        ////////////////////////////////////////////////////////////////////////////////
        // Callbacks
        ////////////////////////////////////////////////////////////////////////////////

        // Receive MSS access acknowledge from Bus Interface Unit
        void sendAck_(const MemoryAccessInfoPtr & mem_access_info_ptr);



        ////////////////////////////////////////////////////////////////////////////////
        // Regular Function/Subroutine Call
        ////////////////////////////////////////////////////////////////////////////////

        // Handle cache access request
        bool handleCacheLookupReq_(const MemoryAccessInfoPtr & mem_access_info_ptr);

        // Access Cache
        bool cacheLookup_(const MemoryAccessInfoPtr &);

        // Reload cache line
        void reloadCache_(uint64_t);

        //An unordered set indexed by the bits of an instruction and containing all its pending (mmu/cache) accesses
        sparta::Counter count_requests_=sparta::Counter(getStatisticSet(), "requests", "Number of requests", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_misses_on_already_pending_=sparta::Counter(getStatisticSet(), "misses_on_already_pending", "Number of misses on addreses that have already been requested", sparta::Counter::COUNT_NORMAL);
        sparta::Counter count_cache_misses_=sparta::Counter(getStatisticSet(), "cache_misses", "Number of cache misses", sparta::Counter::COUNT_NORMAL);
        
        sparta::StatisticDef miss_ratio_{
            getStatisticSet(), "miss_ratio",
            "Miss ratio",
            getStatisticSet(), "cache_misses/requests"
        };

        class L2RequestHash
        {
            public:
                size_t operator()(const L2Request& h) const
                {
                     return h.getAddress();
                }
        };

        class PendingList
        {
            public:
                PendingList(uint64_t l):line_size_(l){}
                
                void insert(L2Request req)
                {
                    misses_.insert(std::make_pair(getLine(req),req));
                }
                
                auto equal_range(L2Request req)
                {
                    return misses_.equal_range(getLine(req));
                }

                void erase(L2Request req)
                {
                    misses_.erase(getLine(req));
                }
            
                bool contains(L2Request req)
                {
                    return misses_.find(getLine(req))!=misses_.end();
                }
                
            private:
                std::unordered_multimap<uint64_t, L2Request> misses_;
                uint64_t line_size_;

                uint64_t getLine(L2Request req)
                {
                    return (req.getAddress()/line_size_)*line_size_;
                }
        };

        PendingList in_flight_reads_;

        PendingList pending_requests_;
    };



} // namespace spike_model

#endif

