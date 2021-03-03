
#include "Request.hpp"

namespace spike_model
{
    uint16_t Request::calculateHome(CacheDataMappingPolicy d, uint8_t tag_bits, uint8_t block_offset_bits, uint8_t set_bits, uint8_t bank_bits)
    {
        uint16_t destination=0;

        if(bank_bits>0) //If more than one bank
        {
            uint8_t left=tag_bits;
            uint8_t right=block_offset_bits;
            switch(d)
            {
                case CacheDataMappingPolicy::SET_INTERLEAVING:
                    left+=set_bits-bank_bits;
                    break;
                case CacheDataMappingPolicy::PAGE_TO_BANK:
                    right+=set_bits-bank_bits;
                    break;
            }
            destination=(getAddress() << left) >> (left+right);
        }
        return destination;
    }

    void Request::setMemoryAccessInfo(uint64_t memory_cpu, uint64_t rank, uint64_t bank, uint64_t row, uint64_t col)
    {
        memory_cpu_=memory_cpu;
        rank_=rank;
        memory_bank_=bank;
        row_=row;
        col_=col;
    }

    uint64_t Request::getMemoryCPU()
    {
        return memory_cpu_;
    }

    uint64_t Request::getRank()
    {
        return rank_;
    }

    uint64_t Request::getMemoryBank()
    {
        return memory_bank_;
    }

    uint64_t Request::getRow()
    {
        return row_;
    }

    uint64_t Request::getCol()
    {
        return col_;
    }
}
