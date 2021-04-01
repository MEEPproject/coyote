
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
    }

    void NoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        count_rx_packets_++;
        switch(mess->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::MEMORY_REQUEST:
            case NoCMessageType::REMOTE_L2_ACK:
            case NoCMessageType::MCPU_REQUEST:
                count_tx_packets_++;
                break;

            default:
                sparta_assert(false, "Unsupported message received from a Tile!!!");
        }
    }

    void NoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        count_rx_packets_++;
        switch(mess->getType())
        {
            case NoCMessageType::MEMORY_ACK:
            case NoCMessageType::MCPU_REQUEST:
                count_tx_packets_++;
                break;

            default:
                sparta_assert(false, "Unsupported message received from a Memory Controller!!!");
        }
    }

} // spike_model
