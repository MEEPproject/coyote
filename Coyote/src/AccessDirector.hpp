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


#ifndef __ACCESS_DIRECTOR_HH__
#define __ACCESS_DIRECTOR_HH__

#include "AddressMappingPolicy.hpp"
#include "CacheDataMappingPolicy.hpp"
#include "VRegMappingPolicy.hpp"
#include "EventVisitor.hpp"

#include <map>

namespace coyote
{
    class Tile; //Forward declaration
    class NoCMessage;

    class AccessDirector : public coyote::EventVisitor
    {
        using coyote::EventVisitor::handle; //This prevents the compiler from warning on overloading 

        /*!
         * \class AccessDirector 
         * \brief The component in charge of forwarding requests to the correct element of the memory hierarchy
         */
        public:
            
            /*!
             * \brief Constructor for AccessDirector
             * \param t The tile that contains the AccessDirector
             */
            AccessDirector(Tile * t) 
                                                    : tile(t), bank_data_mapping_policy_(CacheDataMappingPolicy::PAGE_TO_BANK), 
                                                      scratchpad_data_mapping_policy_(VRegMappingPolicy::CORE_TO_BANK), pending_scratchpad_management_ops(){}
            
            /*!
             * \brief Constructor for AccessDirector
             * \param t The tile that contains the AccessDirector
             * \param b The data mapping policy for cache access
             * \param s The data mapping policy for scratchpad accesses
             */
            AccessDirector(Tile * t, CacheDataMappingPolicy b, VRegMappingPolicy s) 
                                                    : tile(t), bank_data_mapping_policy_(b), 
                                                      scratchpad_data_mapping_policy_(s), pending_scratchpad_management_ops(){}

            /*!
             * \brief Forwards an access to the director
             * \param access The Request to forward
             */
            virtual void putAccess(std::shared_ptr<Request> access);
         
            /*!
             * \brief Handles a cache request
             * \param r The event to handle
             */
            void handle(std::shared_ptr<coyote::CacheRequest> r) override;
            
            /*!
             * \brief Handles a scratchpad request request
             * \param r The event to handle
             */
            void handle(std::shared_ptr<coyote::ScratchpadRequest> r) override;
            
            /*!
             * \brief Set the information on the memory hierarchy
             * \param l2_tile_size The total size of the L2 that belongs to each Tile
             * \param assoc The associativity
             * \param line_size The line size
             * \param banks_per_tile The number of banks per Tile
             * \param the number of tiles in the system
             * \param num_mcs The number of memory controllers
             * \param memory_controller_shift The number of bits to shift to get the memory controller that handles an address
             * \param memory_controller_mask The mask for the AND to extract the memory controller id
             */
            void setMemoryInfo(uint64_t l2_tile_size, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile,  uint16_t num_tiles, 
                                uint64_t num_mcs, uint64_t memory_controller_shift, uint64_t memory_controller_mask, uint16_t cores);
            
            /*!
             * \brief Get a NoCMessage representing a remote L2 Request
             * \param req The request associated to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getRemoteL2RequestMessage(std::shared_ptr<CacheRequest> req);
            /*!
             * \brief Get a NoCMessage representing a memory Request
             * \param req The request associated to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getMemoryRequestMessage(std::shared_ptr<CacheRequest> req);
            
            /*!
             * \brief Get a NoCMessage representing a data forward
             * \param req The request associated to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getDataForwardMessage(std::shared_ptr<CacheRequest> req);

            /*!
             * \brief Get a NoCMessage representing a data forward
             * \param req The request associated to the message
             * \return The NoCMessage
             */
            std::shared_ptr<NoCMessage> getScratchpadAckMessage(std::shared_ptr<ScratchpadRequest> req);

            uint16_t getCoresPerTile();

        protected:
            uint64_t line_size;

            uint8_t tag_bits;
            uint8_t block_offset_bits;
            uint8_t tile_bits;
            uint8_t set_bits;
            uint8_t bank_bits;
            

        private:
            Tile * tile;

            uint64_t num_ways;
            uint16_t num_banks_per_core;
            uint64_t way_size;

            static const uint8_t num_vregs_per_core=32;

            uint8_t vreg_bits;
            uint8_t core_bits;
            uint8_t core_to_bank_interleaving_bits;

            uint64_t scratchpad_ways=0;
            uint64_t scratchpad_available_size=0;
            
            uint64_t mc_shift; //One channel per memory controller
            uint64_t mc_mask;

            const uint8_t address_size=8; //In bytes

            uint16_t cores_per_tile=0;

            /*!              
            * \brief Calculate the home tile and bank for a request
            * \param r A Request
            * \return The home tile
            */
            virtual uint16_t calculateHome(std::shared_ptr<coyote::CacheRequest> r)=0;
            
            /*!              
            * \brief Calculate the bank for a cache request
            * \param r A CacheRequest
            * \return The bank to access
            */
            virtual uint16_t calculateBank(std::shared_ptr<coyote::CacheRequest> r)=0;


            /*!              
            * \brief Calculate the total size of the L2 cache in bytes
	    * \param s The size for the current tile in KB
	    * \param num_tiles The number of tiles in the system
            * \return The total size of the L2 cache in bytes
            */
            virtual uint64_t totalSize(uint64_t s, uint16_t num_tiles)=0;

            /*!              
            * \brief Calculate the bank for a scratchpad request
            * \param r A ScratchpadRequest
            * \return The bank to access
            */
            uint16_t calculateBank(std::shared_ptr<coyote::ScratchpadRequest> r);
            
        protected:
            CacheDataMappingPolicy bank_data_mapping_policy_;
            VRegMappingPolicy scratchpad_data_mapping_policy_;

        private: 
            std::map<std::shared_ptr<coyote::ScratchpadRequest>, uint64_t> pending_scratchpad_management_ops;

    };

}
#endif
