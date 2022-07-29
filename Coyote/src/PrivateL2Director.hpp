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


#ifndef __PRIVATE_L2_DIRECTOR_HH__
#define __PRIVATE_L2_DIRECTOR_HH__

#include <memory>
#include "AccessDirector.hpp"

namespace coyote
{
    class PrivateL2Director : public AccessDirector
    {
        /**
         * \class coyote::PrivateL2Director
         *
         * \brief A request manager for tile-private L2s
         */
        public:
            /*!
             * \brief Constructor for the request manager
             * \param t The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             */
            PrivateL2Director(Tile * t) : AccessDirector(t) {}
            
            /*!
             * \brief Constructor for the request manager
             * \param t The tiles handled my the request manager
             * \param b The cache data mapping policy for banks within a tile
             * \param s The data mapping policy for scratchpad accesses
             */
            PrivateL2Director(Tile * t, CacheDataMappingPolicy b, VRegMappingPolicy s) : AccessDirector(t, b, s) {}
            

        private:
            /*!              
            * \brief Calculate the home tile for a request
            * \param r A Request
            * \return The home tile
            */
            uint16_t calculateHome(std::shared_ptr<coyote::CacheRequest> r) override;
            
            /*!              
            * \brief Calculate the bank for a request
            * \param r A Request
            * \return The bank to access
            */
            uint16_t calculateBank(std::shared_ptr<coyote::CacheRequest> r);
            
	    /*!              
            * \brief Calculate the total size of the L2 cache in bytes
	    * \param s The size for the current tile in KB
	    * \param num_tiles The number of tiles in the system
            * \return The total size of the L2 cache in bytes
            */
            uint64_t totalSize(uint64_t s, uint16_t num_tiles) override;
    };
}
#endif
