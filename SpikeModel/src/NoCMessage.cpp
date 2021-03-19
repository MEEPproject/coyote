#include "NoCMessage.hpp"
#include <math.h>
#include "sparta/utils/SpartaAssert.hpp"

namespace spike_model
{
    NoCMessage::NoCMessage(std::shared_ptr<Event> r, NoCMessageType t, uint64_t payload_size, uint16_t destPort)
    {
        request_=r;
        message_type_=t;
        size_=payload_size+header_size_;
        this->destPort = destPort;
    }    

    uint64_t NoCMessage::getSize()
    {
        return size_;
    }

    void NoCMessage::setDestPort(uint16_t destPort)
    {
        this->destPort = destPort;
    }

}
