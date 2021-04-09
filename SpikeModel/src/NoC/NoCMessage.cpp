#include "NoCMessage.hpp"

namespace spike_model
{
    NoCMessage::NoCMessage(std::shared_ptr<Event> r, NoCMessageType t, uint64_t payload_size, uint16_t src_port, uint16_t dst_port)
    {
        request_=r;
        message_type_=t;
        size_=payload_size+header_size_;
        src_port_ = src_port;
        dst_port_ = dst_port;
    }    

    uint64_t NoCMessage::getSize()
    {
        return size_;
    }

    void NoCMessage::setDstPort(uint16_t dst_port)
    {
        dst_port_ = dst_port;
    }

}
