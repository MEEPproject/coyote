
#ifndef __ADDRESS_MAPPING_SCHEME_HH__
#define __ADDRESS_MAPPING_SCHEME_HH__

namespace spike_model
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
