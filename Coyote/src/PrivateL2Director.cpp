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

#include "PrivateL2Director.hpp"
#include "CacheRequest.hpp"

namespace coyote
{
    uint64_t PrivateL2Director::totalSize(uint64_t s, uint16_t num_tiles)
    {
	return s*1024;
    }

    uint16_t PrivateL2Director::calculateHome(std::shared_ptr<coyote::CacheRequest> r)
    {
        return r->getSourceTile();
    }
            
    uint16_t PrivateL2Director::calculateBank(std::shared_ptr<coyote::CacheRequest> r)
    {
        uint16_t destination=0;
        
        if(bank_bits>0) //If more than one bank
        {
            uint8_t left=tag_bits;
            uint8_t right=block_offset_bits;
            switch(bank_data_mapping_policy_)
            {
                case CacheDataMappingPolicy::SET_INTERLEAVING:
                    left+=set_bits-bank_bits;
                    break;
                case CacheDataMappingPolicy::PAGE_TO_BANK:
                    right+=set_bits-bank_bits;
                    break;
            }
            destination=(r->getAddress() << left) >> (left+right);
        }
        return destination;
    }
}
