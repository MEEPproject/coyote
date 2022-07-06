#ifndef __STALL_REASON_HH__
#define __STALL_REASON_HH__
    
enum class StallReason{
    FETCH_MISS,
    RAW,
    MSHRS,
    WAITING_ON_BARRIER,
    CORE_FINISHED,
    VECTOR_WAITING_ON_SCALAR_STORE,
    MAX_REASONS
};

#endif
