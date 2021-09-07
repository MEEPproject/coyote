#include "NoCMessage.hpp"
#include "NoC.hpp"

#define BYTES_TO_BITS 8

namespace spike_model
{
    uint8_t NoCMessage::header_size[static_cast<int>(NoCMessageType::count)]; // Define header_size for each packet

    NoCMessage::NoCMessage(std::shared_ptr<Event> r, NoCMessageType t, uint16_t payload_size, uint16_t src_port, uint16_t dst_port) :
        request_(r),
        message_type_(t),
        src_port_(src_port),
        dst_port_(dst_port)
    {
        size_= header_size[static_cast<int>(message_type_)] + payload_size + payload_size*BYTES_TO_BITS; // Header (including CRC) + payload + payload ECC
        // Set the noc_network and class
        noc_network_ = static_cast<uint8_t>(NoC::getNetworkForMessage(message_type_));
        class_ = NoC::getClassForMessage(message_type_);
    }

    void NoCMessage::setDstPort(uint16_t dst_port)
    {
        dst_port_ = dst_port;
    }

} // spike_model
