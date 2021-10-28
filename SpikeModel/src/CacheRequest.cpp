
#include "CacheRequest.hpp"

namespace spike_model
{
    void CacheRequest::setBankInfo(uint64_t rank, uint64_t bank, uint64_t row, uint64_t col)
    {
        rank_=rank;
        memory_bank_=bank;
        row_=row;
        col_=col;
    }

    void CacheRequest::setMemoryController(uint64_t memory_controller)
    {
        memory_controller_=memory_controller;
    }
    
    uint64_t CacheRequest::getMemoryController()
    {
        return memory_controller_;
    }

    uint64_t CacheRequest::getRank()
    {
        return rank_;
    }

    uint64_t CacheRequest::getMemoryBank()
    {
        return memory_bank_;
    }

    uint64_t CacheRequest::getRow()
    {
        return row_;
    }

    uint64_t CacheRequest::getCol()
    {
        return col_;
    }
    
    bool CacheRequest::getBypassL1()
    {
        return bypass_l1;
    }
            
    bool CacheRequest::getBypassL2()
    {
        return bypass_l2;
    }
}
