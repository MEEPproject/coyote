
#include "L2Request.hpp"

namespace spike_model
{
    uint16_t L2Request::calculateHome(DataMappingPolicy d, uint8_t tag_bits, uint8_t block_offset_bits, uint8_t set_bits, uint8_t bank_bits)
    {
        uint16_t destination=0;
        
        if(bank_bits>0) //If more than one bank
        {
            uint8_t left=tag_bits;
            uint8_t right=block_offset_bits;
            switch(d)
            {
                case DataMappingPolicy::SET_INTERLEAVING:
                    left+=set_bits-bank_bits;
                    break;
                case DataMappingPolicy::PAGE_TO_BANK:
                    right+=set_bits-bank_bits;
                    break;
            }
            destination=(getAddress() << left) >> (left+right);
        }
        return destination;
    }
}
