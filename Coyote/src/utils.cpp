#include "utils.hpp"

namespace utils 
{
    uint64_t nextPowerOf2(uint64_t v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v++;
        return v;
    }

    std::string reason_to_string(StallReason r)
    {
        switch(r)
        {
            case StallReason::FETCH_MISS:
                return "fetch_miss";
            case StallReason::RAW:
                return "raw";
            case StallReason::MSHRS:
                return "mshrs";
            case StallReason::WAITING_ON_BARRIER:
                return "waiting_on_barrier";
            case StallReason::CORE_FINISHED:
                return "core_finished";
            case StallReason::VECTOR_WAITING_ON_SCALAR_STORE:
                return "vector_waiting_on_scalar_store";
            default:
                return "unknown_reason";
        }
    }
}

