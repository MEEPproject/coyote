#include "NoCMessage.hpp"
#include <math.h>
#include "sparta/utils/SpartaAssert.hpp"

namespace spike_model
{
    NoCMessage::NoCMessage(std::shared_ptr<L2Request> r, NoCMessageType t, uint64_t payload_size)
    {
        request_=r;
        message_type_=t;
        size_=payload_size+header_size_;
    }    

    uint16_t NoCMessage::getMemoryController(uint16_t num_controllers)
    {
        sparta_assert(message_type_==NoCMessageType::MEMORY_REQUEST);
        uint16_t destination;
        destination=request_->getHomeTile()%num_controllers;
        return destination;
    }

    uint64_t NoCMessage::getSize()
    {
        return size_;
    }
}
