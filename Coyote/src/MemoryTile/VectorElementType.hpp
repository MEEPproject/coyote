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

#ifndef __VECTOR_ELEMENT_TYPE_HPP__
#define __VECTOR_ELEMENT_TYPE_HPP__


namespace coyote {
    /**
     * This class defines the types of supported vector elements.
     */
    enum class VectorElementType {
		BIT8	= 1,
		BIT16	= 2,
		BIT32	= 4,
		BIT64	= 8 //The order of this enum should not be changed. The correct assignation of the width in riscv-isa-sim depends on it
	};
	
	enum class LMULSetting {
		EIGHTTH	= -3, // Not 
		FOURTH	= -2, // supported
		HALF 	= -1, // in RVV 0.8
		ONE 	= 0,
		TWO 	= 1,
		FOUR 	= 2,
		EIGHT 	= 3 
	};
}
#endif
