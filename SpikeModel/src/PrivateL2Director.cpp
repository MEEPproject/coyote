#include "PrivateL2Director.hpp"
#include "CacheRequest.hpp"

namespace spike_model
{
    uint16_t PrivateL2Director::calculateHome(std::shared_ptr<spike_model::CacheRequest> r)
    {
        return r->getSourceTile();
    }
            
    uint16_t PrivateL2Director::calculateBank(std::shared_ptr<spike_model::CacheRequest> r)
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
