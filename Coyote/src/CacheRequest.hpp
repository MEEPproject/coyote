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

#ifndef __CACHE_REQUEST_HH__
#define __CACHE_REQUEST_HH__

#include "CacheDataMappingPolicy.hpp"
#include "Request.hpp"
#include <iostream>

namespace coyote
{
    class CacheRequest : public Request, public std::enable_shared_from_this<CacheRequest>
    {
        /**
         * \class coyote::CacheRequest
         *
         * \brief CacheRequest contains all the information regarding a request to the L2 cache.
         *
         * Instances of request are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units, either as plain CacheRequests or encapsulated in a NoCMessage.
         *
         */
        
        friend class AccessDirector; 
        friend class MemoryCPUWrapper; 
        friend class MemoryController; 
    
        public:
            enum class AccessType
            {
                LOAD,
                STORE,
                FETCH,
                WRITEBACK,
            };
            
            //CacheRequest(){}
            CacheRequest() = delete;
            CacheRequest& operator=(Request const&) = delete;

             /*!
             * \brief Constructor for CacheRequest
             * \param  a The requested address
             * \param  t The type of the request
             * \note This is an incomplete cache request used for writebacks. The id of the producing core NEEDS to be set afterwards. Handle with care
             */
            CacheRequest(uint64_t a, AccessType t): Request(a, 0, 0), type(t), mem_tile((uint16_t)-1), memory_ack(false), bypass_l1(false), bypass_l2(false), size_requested_to_memory(0) {}


            /*!
             * \brief Constructor for CacheRequest
             * \param a The requested address
             * \param t The type of the request
             * \param pc The program counter of the requesting instruction
             * \param time The timestamp for the request
             * \param c The requesting core
             * \param  bypass_l2 True if the L2 has to be bypassed. Default value False
             */
            CacheRequest(uint64_t a, AccessType t, uint64_t pc, uint64_t time, uint16_t c, bool bypass_l1=false, bool bypass_l2=false): Request(a, pc, time, c), type(t), mem_tile((uint16_t)-1), memory_ack(false), bypass_l1(bypass_l1), bypass_l2(bypass_l2), size_requested_to_memory(0) {}

            /*!
             * \brief Get the type of the request
             * \return The type of the request
             */
            AccessType getType() const {return type;}


            /*!
             * \bref Set the home tile of the request
             * \param home The home tile
             */
            void setHomeTile(uint16_t home) {home_tile=home;}

            /*!
             * \brief Get the home tile for the request
             * \return The home tile
             */
            uint16_t getHomeTile(){return home_tile;}
            
            /*!
             * \brief Set the source memory tile. This field is used to differentiate
             * incoming transaction in the memory tile. The source could not only be
             * a home tile (see home_tile), but also another memory tile. If a
             * Memory Tile determines, that the address range is not served by it, then
             * this message is forwarded to the Memory Tile which owns that address
             * range. However, that Memory Tile has to know, that the results have to
             * be returned to the original Memory Tile, that was contacted first by
             * the VAS Tile.
             * \param mem_tile The memory tile where the request originated.
             */
            void setMemTile(uint16_t mem_tile) {this->mem_tile = mem_tile;}
            
            /*!
             * \brief Returning the memory tile, where the request originated.
             * \return The ID of the originate Memory Tile
             */
            uint16_t getMemTile() {return mem_tile;}
            
            /*!
             * \brief Get the memory controller that will be accessed
             * \return The address of the line
             */
            uint64_t getMemoryController();
            
            /*!
             * \brief Get the rank taht will be accessed
             * \return The address of the line
             */
            uint64_t getRank();
            
            /*!
             * \brief Get the bank that will be accessed
             * \return The address of the line
             */
            uint64_t getMemoryBank();
            
            /*!
             * \brief Get the row that will be accessed
             * \return The address of the line
             */
            uint64_t getRow();
            
            /*!
             * \brief Get whether the L1 has to be bypassed
             * \return True if the L1 has to be bypassed
             */
            bool getBypassL1();

            /*!
             * \brief Get the column that will be accessed
             * \return The address of the line
             */
            uint64_t getCol();

            /*!
             * \brief Get whether the L2 has to be bypassed
             * \return True if the L2 has to be bypassed
             */
            bool getBypassL2();

            /*
             * \brief Equality operator for instances of CacheRequest
             * \param m The request to compare with
             * \return true if both requests are for the same address
             */
            bool operator ==(const CacheRequest & m) const
            {
                return m.getAddress()==getAddress();
            }

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

            bool memoryAck()
            {
                return memory_ack;
            }

            void setMemoryAck(bool ack)
            {
                memory_ack = ack;
            }
    
            void setProducedByVector()
            {
                produced_by_vector_instruction=true;
            }

            bool getProducedByVector()
            {
                return produced_by_vector_instruction;
            }

            /*
             * \brief Get the number of bytes that have been handled by memory for this cache request
             * \return The number of bytes handled by memory
             */
            uint16_t getSizeRequestedToMemory();

            /*
             * \brief Increases the number of bytes that have been handled by memory for this cache request
             * \param s The number of bytes to increase
             */
            void increaseSizeRequestedToMemory(uint16_t s);
            
            /*
             * \brief Check if the request is allocating
             * \return True if the request is allocating
             */
            bool isAllocating();

            /*
             * \brief Set the request as allocating
             */
            void setAllocate();

            void setClosesMemoryRow();

            void setMissesMemoryRow();

            bool getClosesMemoryRow();

            bool getMissesMemoryRow();

        private:
            AccessType type;

            uint16_t home_tile;
            uint16_t mem_tile;

            uint64_t memory_controller_;
            uint64_t rank_;
            uint64_t memory_bank_;
            uint64_t row_;
            uint64_t col_;
            bool memory_ack;

            bool bypass_l1;
            bool bypass_l2;
            
            uint16_t size_requested_to_memory;
            bool allocating=false;

            bool closes_memory_row=false;
            bool misses_memory_row=false;

            bool produced_by_vector_instruction=false;

            /*!
             * \brief Set the bank information of the memory access triggered by the CacheRequest
             * \param rank The rank that will be accessed
             * \param bank The bank that will be accessed
             * \param row The row that will be accessed
             * \param col The column that will be accessed
             * \note This method is private but called through friending by instances of MemoryController
             */
            void setBankInfo(uint64_t rank, uint64_t bank, uint64_t row, uint64_t col);
            
            /*!
             * \brief Set the information of the memory access triggered by the CacheRequest
             * \param memory_controller The memory controller that will be accessed
             * \note This method is private but called through friending by instances of AccessDirector
             */
            void setMemoryController(uint64_t memory_controller);

            
            /*
             * \brief Reset the number of bytes that have been handled by memory for this cache request
             */
            void resetSizeRequestedToMemory();
    };
    
    inline std::ostream& operator<<(std::ostream &str, CacheRequest &req)
    {
        str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp() << ", coreID: 0x" << req.getCoreId() << ", destRegID: " << req.getDestinationRegId() << ", destRegType: " << (int)req.getDestinationRegType() << ", type: " << (int)req.getType() << ", memTile: " << (int)req.getMemTile() << ", serviced: " << req.isServiced();
        return str;
    }
}
#endif
