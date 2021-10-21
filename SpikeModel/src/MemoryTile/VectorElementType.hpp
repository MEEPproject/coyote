#ifndef __VECTOR_ELEMENT_TYPE_HPP__
#define __VECTOR_ELEMENT_TYPE_HPP__


namespace spike_model {
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
