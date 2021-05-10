#include "NoCMessage.hpp"
#include "NoC.hpp"

#define BYTES_TO_BITS 8

namespace spike_model
{
    uint8_t NoCMessage::header_size = 8; // Define header_size with a default value

    NoCMessage::NoCMessage(std::shared_ptr<Event> r, NoCMessageType t, uint16_t payload_size, uint16_t src_port, uint16_t dst_port) :
        request_(r),
        message_type_(t),
        src_port_(src_port),
        dst_port_(dst_port)
    {
        size_= header_size + payload_size*BYTES_TO_BITS + payload_size; // Header (including ECC) + payload + payload ECC
        // Fill the transaction_type field that currently represents the destination NoC network
        switch(message_type_)
        {
            // Data-Transfer NoC
            case NoCMessageType::MEMORY_REQUEST_STORE:
                transaction_type_ = static_cast<uint8_t>(NoC::Networks::DATA_TRANSFER_NOC);
                priority_ = 0;
                break;
            case NoCMessageType::REMOTE_L2_ACK:
            case NoCMessageType::MEMORY_ACK:
                transaction_type_ = static_cast<uint8_t>(NoC::Networks::DATA_TRANSFER_NOC);
                priority_ = 1;
                break;
            case NoCMessageType::SCRATCHPAD_DATA_REPLY:
                transaction_type_ = static_cast<uint8_t>(NoC::Networks::DATA_TRANSFER_NOC);
                priority_ = 2;
                break;
            case NoCMessageType::SCRATCHPAD_COMMAND:
                transaction_type_ = static_cast<uint8_t>(NoC::Networks::DATA_TRANSFER_NOC);
                priority_ = 3;
                break;

            // Address-only NoC
            case NoCMessageType::MCPU_REQUEST:
                transaction_type_ = static_cast<uint8_t>(NoC::Networks::ADDRESS_ONLY_NOC);
                priority_ = 0;
                break;
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::MEMORY_REQUEST:
            case NoCMessageType::MEMORY_REQUEST_LOAD:
                transaction_type_ = static_cast<uint8_t>(NoC::Networks::ADDRESS_ONLY_NOC);
                priority_ = 1;
                break;

            // Control NoC
            case NoCMessageType::SCRATCHPAD_ACK:
                transaction_type_ = static_cast<uint8_t>(NoC::Networks::CONTROL_NOC);
                priority_ = 0;
                break;

            default:
                sparta_assert(false);
        }
    }

    void NoCMessage::setDstPort(uint16_t dst_port)
    {
        dst_port_ = dst_port;
    }

} // spike_model
