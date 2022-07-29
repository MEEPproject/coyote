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


#ifndef __ADDRESS_MAPPING_SCHEME_HH__
#define __ADDRESS_MAPPING_SCHEME_HH__

namespace coyote
{
    enum class AddressMappingPolicy
    {
        OPEN_PAGE,
        CLOSE_PAGE,
        ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE, //Implemented accrding to the XILINX spec
        ROW_COLUMN_BANK, //Implemented accrding to the XILINX spec
        BANK_ROW_COLUMN //Implemented accrding to the XILINX spec
    };
}

#endif
