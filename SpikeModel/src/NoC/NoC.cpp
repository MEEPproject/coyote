
#include "NoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"
#include <chrono>

namespace spike_model
{
    const char NoC::name[] = "noc";

    NoC::NoC(sparta::TreeNode *node, const NoCParameterSet *params) :
        sparta::Unit(node),
        in_ports_tiles_(params->num_tiles),
        out_ports_tiles_(params->num_tiles),
        in_ports_memory_cpus_(params->num_memory_cpus),
        out_ports_memory_cpus_(params->num_memory_cpus),
        noc_model_(params->noc_model),
        num_tiles_(params->num_tiles),
        num_memory_cpus_(params->num_memory_cpus)
    {
        for(uint16_t i=0; i<num_tiles_; i++)
        {
            std::string out_name=std::string("out_tile") + sparta::utils::uint32_to_str(i);
            std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, out_name);
            out_ports_tiles_[i]=std::move(out);

            std::string in_name=std::string("in_tile") + sparta::utils::uint32_to_str(i);
            std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>> in=std::make_unique<sparta::DataInPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, in_name);
            in_ports_tiles_[i]=std::move(in);
            in_ports_tiles_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(NoC, handleMessageFromTile_, std::shared_ptr<NoCMessage>));
        }

        for(uint16_t i=0; i<num_memory_cpus_; i++)
        {
            std::string out_name=std::string("out_memory_cpu") + sparta::utils::uint32_to_str(i);
            std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, out_name);
            out_ports_memory_cpus_[i]=std::move(out);

            std::string in_name=std::string("in_memory_cpu") + sparta::utils::uint32_to_str(i);
            std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>> in=std::make_unique<sparta::DataInPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, in_name);
            in_ports_memory_cpus_[i]=std::move(in);
            in_ports_memory_cpus_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(NoC, handleMessageFromMemoryCPU_, std::shared_ptr<NoCMessage>));
        }

        // Set the messages' header size
        NoCMessage::header_size = params->header_size;
    }

    void NoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Packet counter for each network
        switch(static_cast<Networks>(mess->getTransactionType()))
        {
            case Networks::DATA_TRANSFER_NOC:
                count_rx_packets_data_transfer_++;
                count_tx_packets_data_transfer_++;
                break;
            case Networks::ADDRESS_ONLY_NOC:
                count_rx_packets_address_only_++;
                count_tx_packets_address_only_++;
                break;
            case Networks::CONTROL_NOC:
                count_rx_packets_control_++;
                count_tx_packets_control_++;
                break;
            default:
                sparta_assert(false, "Unsupported network in NoC");
        }
        // Packet counter by each type
        switch(mess->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
                count_remote_l2_requests_++;
                break;
            case NoCMessageType::MEMORY_REQUEST:
                count_memory_requests_++;
                break;
            case NoCMessageType::MEMORY_REQUEST_LOAD:
                count_memory_requests_load_++;
                break;
            case NoCMessageType::MEMORY_REQUEST_STORE:
                count_memory_requests_store_++;
                break;
            case NoCMessageType::REMOTE_L2_ACK:
                count_remote_l2_acks_++;
                break;
            case NoCMessageType::MCPU_REQUEST:
                count_mcpu_requests_++;
                break;
            case NoCMessageType::SCRATCHPAD_ACK:
                count_scratchpad_acks_++;
                break;
            case NoCMessageType::SCRATCHPAD_DATA_REPLY:
                count_scratchpad_data_replies_++;
                break;
            default:
                sparta_assert(false, "Unsupported message received from a Tile!!!");
        }
    }

    void NoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Packet counter for each network
        switch(static_cast<Networks>(mess->getTransactionType()))
        {
            case Networks::DATA_TRANSFER_NOC:
                count_rx_packets_data_transfer_++;
                count_tx_packets_data_transfer_++;
                break;
            case Networks::ADDRESS_ONLY_NOC:
                count_rx_packets_address_only_++;
                count_tx_packets_address_only_++;
                break;
            case Networks::CONTROL_NOC:
                count_rx_packets_control_++;
                count_tx_packets_control_++;
                break;
            default:
                sparta_assert(false, "Unsupported network in NoC");
        }
        // Packet counter by each type
        switch(mess->getType())
        {
            case NoCMessageType::MEMORY_ACK:
                count_memory_acks_++;
                break;
            case NoCMessageType::MCPU_REQUEST:
                count_mcpu_requests_++;
                break;
            case NoCMessageType::SCRATCHPAD_COMMAND:
                count_scratchpad_commands_++;
                break;
            default:
                sparta_assert(false, "Unsupported message received from a MCPU!!!");
        }
    }

} // spike_model
