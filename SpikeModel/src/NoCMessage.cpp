#include "NoCMessage.hpp"
#include <math.h>

namespace spike_model
{
    uint16_t NoCMessage::getMemoryController(uint16_t num_controllers)
    {
        uint16_t destination;
        if(num_controllers==1)
        {
            destination=0;
        }
        else
        {
            destination=request_->getAddress() >> (64 - (uint16_t)ceil(log2(num_controllers)));
        }
        return destination;
    }
}
