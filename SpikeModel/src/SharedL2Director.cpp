#include "SharedL2Director.hpp"
#include "CacheRequest.hpp"

namespace spike_model
{
    uint16_t SharedL2Director::calculateHome(std::shared_ptr<spike_model::CacheRequest> r)
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
    
    uint16_t SharedL2Director::calculateBank(std::shared_ptr<spike_model::CacheRequest> r)
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
