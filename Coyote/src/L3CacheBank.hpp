// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 


#ifndef __L3CacheBank_H__
#define __L3CacheBank_H__

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
#include "CacheBank.hpp"
#include "ScratchpadRequest.hpp"

#include "SimpleDL2.hpp"

#include "LogCapable.hpp"

namespace coyote
{
    class L3CacheBank : public CacheBank
    {
        /*!
         * \class coyote::CacheBank
         * \brief A cache bank that belongs to a Tile in the architecture.
         *
         * Only one request is issued into the cache per cycle, but up to max_outstanding_misses_ might
         * be in service at the some time. Whether banks are shared or private to the tile is controlled from
         * the FullSystemSimulationEventManager class. The Cache is write-back and write-allocate.
         *
         * This cache might return more than one ack in the same cycle if an access that corresponds to more than one request is serviced. 
         * External arbitration and queueing is necessary to avoid this behavior.
         *
         */
    public:
        /*!
         * \class CacheBankParameterSet
         * \brief Parameters for CacheBank model
         */
        class L3CacheBankParameterSet : public sparta::ParameterSet
        {
        public:
            //! Constructor for CacheBankParameterSet
            L3CacheBankParameterSet(sparta::TreeNode* n):
                sparta::ParameterSet(n)
            {
            }

            PARAMETER(uint64_t, line_size, 128, "DL1 line size (power of 2)")
            PARAMETER(uint64_t, size_kb, 2048, "Size of DL1 in KB (power of 2)")
            PARAMETER(uint64_t, associativity, 8, "DL1 associativity (power of 2)")
            PARAMETER(uint32_t, bank_and_tile_offset, 1, "The stride for banks and tiles. It is log2'd to get the bits that need to be shifted to identify the set")
            PARAMETER(bool, always_hit, false, "DL1 will always hit")
            // Parameters for event scheduling
            PARAMETER(uint16_t, miss_latency, 10, "Cache miss latency")
            PARAMETER(uint16_t, hit_latency, 10, "Cache hit latency")
            PARAMETER(uint16_t, max_outstanding_misses, 8, "Maximum misses in flight to the next level")
            PARAMETER(uint16_t, max_outstanding_wbs, 1, "Maximum number of in flight wbs")
            PARAMETER(bool, unit_test, false, "The bank will be used in a unit testing scenario")
        };

        /*!
         * \brief Constructor for CacheBank
         * \param node The node that represent the CacheBank and
         * \param p The CacheBank parameter set
         */
        L3CacheBank(sparta::TreeNode* node, const L3CacheBankParameterSet* p);

        ~L3CacheBank() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << memory_access_allocator.getNumAllocated()
                          << " MemoryAccessInfo objects allocated/created"
                          << std::endl;
        }

        void scheduleIssueAccess(uint64_t cycle);
        virtual void getAccess_(const std::shared_ptr<Request> & req) override;
        //! name of this resource.
        static const char name[];
        void issueAccess_();
        sparta::UniqueEvent<> issue_access_event_
            {&unit_event_set_, "issue_access_", CREATE_SPARTA_HANDLER(L3CacheBank, issueAccess_)};

        void sendAck_(const std::shared_ptr<CacheRequest> & req);
        sparta::PayloadEvent<std::shared_ptr<CacheRequest> > send_ack_event_ {&unit_event_set_, "send_ack_event_", CREATE_SPARTA_HANDLER_WITH_DATA(L3CacheBank, sendAck_, std::shared_ptr<CacheRequest> )};

      private:
        virtual void logCacheRequest(std::shared_ptr<CacheRequest> r) override;
    };
} // namespace coyote

#endif

