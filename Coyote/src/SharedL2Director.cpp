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

#include "SharedL2Director.hpp"
#include "CacheRequest.hpp"

namespace coyote
{
    uint64_t SharedL2Director::totalSize(uint64_t s, uint16_t num_tiles)
    {
	return s*1024*num_tiles;
    }

    uint16_t SharedL2Director::calculateHome(std::shared_ptr<coyote::CacheRequest> r)
    {
        uint16_t destination=0;
        
        if(tile_bits>0) //If more than one bank
        {
            uint8_t left=tag_bits;
            uint8_t right=block_offset_bits;
            switch(tile_data_mapping_policy_)
            {
                case CacheDataMappingPolicy::SET_INTERLEAVING:
                    left+=set_bits-tile_bits;
                    break;
                case CacheDataMappingPolicy::PAGE_TO_BANK:
                    right+=set_bits-tile_bits;
                    break;
            }
            destination=(r->getAddress() << left) >> (left+right);
        }
        return destination;
    }
    
    uint16_t SharedL2Director::calculateBank(std::shared_ptr<coyote::CacheRequest> r)
    {
        uint16_t destination=0;
        
        if(bank_bits>0) //If more than one bank
        {
            uint8_t left=tag_bits;
            uint8_t right=block_offset_bits;
            switch(tile_data_mapping_policy_)
            {
                case CacheDataMappingPolicy::SET_INTERLEAVING:
                    switch(bank_data_mapping_policy_)
                    {
                        case CacheDataMappingPolicy::SET_INTERLEAVING:
                            //We are interleaving between both tiles and banks
                            left+=set_bits-(bank_bits+tile_bits);
                            right+=tile_bits;
                            break;

                        case CacheDataMappingPolicy::PAGE_TO_BANK:
                            //In a tile, sets belonging to the same page go to the same bank
                            right+=set_bits-bank_bits;
                            break;
                    }
                    break;

                case CacheDataMappingPolicy::PAGE_TO_BANK:
                    switch(bank_data_mapping_policy_)
                    {
                        case CacheDataMappingPolicy::PAGE_TO_BANK:
                            //Page goest to a tile and to a particular bank
                            left+=tile_bits;
                            right+=set_bits-(bank_bits+tile_bits);
                            break;

                        case CacheDataMappingPolicy::SET_INTERLEAVING:
                            //Page goest to a tile and is interleaved in the bank bank
                            left+=set_bits-bank_bits;
                            break;
                    }
                    break;

            }
            destination=(r->getAddress() << left) >> (left+right);
        }
        return destination;

    }
}
