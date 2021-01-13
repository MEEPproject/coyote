#include "NoCMessage.hpp"
#include <math.h>
#include "sparta/utils/SpartaAssert.hpp"

namespace spike_model
{
    NoCMessage::NoCMessage(std::shared_ptr<Request> r, NoCMessageType t, uint64_t payload_size)
    {
        request_=r;
        message_type_=t;
        size_=payload_size+header_size_;
    }    

    uint64_t NoCMessage::getSize()
    {
        return size_;
    }
}
