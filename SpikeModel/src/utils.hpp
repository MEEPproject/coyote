
#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include "StallReason.hpp"
#include <string>

namespace utils 
{
    uint64_t nextPowerOf2(uint64_t v);
    std::string reason_to_string(StallReason r);
}
#endif

