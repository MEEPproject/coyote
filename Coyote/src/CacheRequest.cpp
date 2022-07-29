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


#include "CacheRequest.hpp"

namespace coyote
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
            
    uint16_t CacheRequest::getSizeRequestedToMemory()
    {
        return size_requested_to_memory;
    }
 
    void CacheRequest::increaseSizeRequestedToMemory(uint16_t s)
    {
        size_requested_to_memory+=s;
    }
    
    void CacheRequest::resetSizeRequestedToMemory()
    {
        size_requested_to_memory=0;
    }

    bool CacheRequest::isAllocating()
    {
        return allocating;
    }

    void CacheRequest::setAllocate()
    {
        resetSizeRequestedToMemory();
        allocating=true;
    }
            
    void CacheRequest::setClosesMemoryRow()
    {
        closes_memory_row=true;
    }

    void CacheRequest::setMissesMemoryRow()
    {
        misses_memory_row=true;
    }

    bool CacheRequest::getClosesMemoryRow()
    {
        return closes_memory_row;
    }

    bool CacheRequest::getMissesMemoryRow()
    {
        return misses_memory_row;
    }            
}
