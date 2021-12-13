
#include "NoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"
#include <chrono>

namespace spike_model
{
    const char NoC::name[] = "noc";
    std::map<NoCMessageType,std::pair<uint8_t,uint8_t>> NoC::message_to_network_and_class_ = std::map<NoCMessageType,std::pair<uint8_t,uint8_t>>();

    NoC::NoC(sparta::TreeNode *node, const NoCParameterSet *params) :
        sparta::Unit(node),
        in_ports_tiles_(params->num_tiles),
        out_ports_tiles_(params->num_tiles),
        in_ports_memory_cpus_(params->num_memory_cpus),
        out_ports_memory_cpus_(params->num_memory_cpus),
        num_tiles_(params->num_tiles),
        num_memory_cpus_(params->num_memory_cpus),
        x_size_(params->x_size),
        y_size_(params->y_size),
        size_(num_tiles_ + num_memory_cpus_),
        mcpus_indices_(params->mcpus_indices),
        noc_model_(params->noc_model),
        max_class_used_(0),
        noc_networks_(params->noc_networks)
    {
        for(uint16_t i=0; i<num_tiles_; i++)
        {
            std::string out_name=std::string("out_tile") + sparta::utils::uint32_to_str(i);
            std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, out_name);
            out_ports_tiles_[i]=std::move(out);

            std::string in_name=std::string("in_tile") + sparta::utils::uint32_to_str(i);
            std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>> in=std::make_unique<sparta::DataInPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, in_name, sparta::SchedulingPhase::PostTick, 0);
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

        sparta_assert(x_size_ * y_size_ == num_memory_cpus_ + num_tiles_, 
            "The NoC mesh size must be equal to the number of elements to connect:" << 
            "\n X: " << x_size_ <<
            "\n Y: " << y_size_ <<
            "\n PEs: " << num_memory_cpus_ + num_tiles_);
        sparta_assert(x_size_ * y_size_ == size_);
        sparta_assert(params->mcpus_indices.isVector(), "The top.cpu.params.mcpus_indices must be a vector");
        sparta_assert(params->mcpus_indices.getNumValues() == num_memory_cpus_, 
            "The number of elements in mcpus_indices must be equal to the number of MCPUs");

        // Validate NoC Networks name
        sparta_assert(noc_networks_.size() < std::numeric_limits<uint8_t>::max());
        for(int i=1; i < static_cast<int>(noc_networks_.size()); i++)
            sparta_assert(noc_networks_[i] != noc_networks_[i-1],
                            "The name of each NoC network in noc_networks must be unique.");

        // Set the messages' header size
        sparta_assert(params->message_header_size.getNumValues() == static_cast<int>(NoCMessageType::count),
                        "The number of messages defined in message_header_size param is not correct.");
        int name_length;
        int size;
        for(auto mess_header_size : params->message_header_size){
            name_length = mess_header_size.find(":");
            size = stoi(mess_header_size.substr(name_length+1));
            sparta_assert(size < std::numeric_limits<uint8_t>::max(),
                            "The header size should be lower than " + std::to_string(std::numeric_limits<uint8_t>::max()));
            NoCMessage::header_size[static_cast<int>(getMessageTypeFromString_(mess_header_size.substr(0,name_length)))] = size;
        }

        // Define the mapping of NoC Messages to Networks and classes
        sparta_assert(params->message_to_network_and_class.getNumValues() == static_cast<int>(NoCMessageType::count),
                        "The number of messages defined in message_to_network_and_class param is not correct.");
        int mess_length;
        int network_length;
        int dot_pos;
        int class_value;
        for(auto mess_net_class : params->message_to_network_and_class){
            mess_length = mess_net_class.find(":");
            dot_pos = mess_net_class.find(".");
            network_length = dot_pos - (mess_length+1);
            class_value = stoi(mess_net_class.substr(dot_pos+1));
            sparta_assert(class_value >= 0 && class_value < std::numeric_limits<uint8_t>::max());
            if(class_value > max_class_used_)
                max_class_used_ = class_value;
            NoC::message_to_network_and_class_[getMessageTypeFromString_(mess_net_class.substr(0,mess_length))] = 
                std::make_pair(getNetworkFromString(mess_net_class.substr(mess_length+1,network_length)), class_value);
        }
        sparta_assert(NoC::message_to_network_and_class_.size() == static_cast<int>(NoCMessageType::count));

        // Statistics
        for(auto network : noc_networks_)
        {
            count_rx_packets_.push_back(sparta::Counter(
                getStatisticSet(),                                       // parent
                "received_packets_" + network,                          // name
                "Number of packets received in " + network + " NoC",    // description
                sparta::Counter::COUNT_NORMAL                            // behavior
            ));
            count_tx_packets_.push_back(sparta::Counter(
                getStatisticSet(),                                       // parent
                "sent_packets_" + network,                               // name
                "Number of packets sent in " + network + " NoC",        // description
                sparta::Counter::COUNT_NORMAL                            // behavior
            ));
            
            load_pkt_.push_back(sparta::StatisticDef(
                getStatisticSet(),                                       // parent
                "load_pkt_" + network,                                   // name
                "Load in " + network + " NoC (pkt/node/cycle)",          // description
                getStatisticSet(),                                       // context
                "received_packets_" + network + "/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
            ));
        }
        for(int i=0; i < static_cast<int>(NoCMessageType::count); i++)
        {
            count_pkts_by_pkt_type.push_back(sparta::Counter(
                getStatisticSet(),                                          // parent
                "num_" + static_cast<NoCMessageType>(i),                   // name
                "Number of " + static_cast<NoCMessageType>(i) + " packets", // description
                sparta::Counter::COUNT_NORMAL                               // behavior
            ));
        }
    }

    uint8_t NoC::getNetworkFromString(const std::string& net)
    {
        for(uint8_t i=0; i < noc_networks_.size(); i++)
        {
            if(noc_networks_[i] == net)
                return i;
        }
        sparta_assert(false, "Network " + net + " not valid, see noc_networks parameter.");
    }

    uint8_t NoC::getNetworkForMessage(const NoCMessageType mess) {return message_to_network_and_class_[mess].first;}
    uint8_t NoC::getClassForMessage(const NoCMessageType mess) {return message_to_network_and_class_[mess].second;}

    void NoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Packet counter for each network
        count_rx_packets_[mess->getNoCNetwork()]++;
        count_tx_packets_[mess->getNoCNetwork()]++;

        // Packet counter by each message type
        count_pkts_by_pkt_type[static_cast<int>(mess->getType())]++;
        switch(mess->getType())
        {
            // VAS -> VAS messages
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::REMOTE_L2_ACK:
            // VAS -> MEM messages
            case NoCMessageType::MEMORY_REQUEST_LOAD:
            case NoCMessageType::MEMORY_REQUEST_STORE:
            case NoCMessageType::MEMORY_REQUEST_WB:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_ACK:
            case NoCMessageType::SCRATCHPAD_DATA_REPLY:
                break;
            default:
                sparta_assert(false, "Unsupported message received from a Tile!!!");
        }

        if(trace_)
        {
            traceSrcDst_(mess);
        }
    }

    void NoC::traceSrcDst_(const std::shared_ptr<NoCMessage> & mess)
    {
        uint32_t dst_id=mess->getDstPort();
        uint32_t src_id=mess->getSrcPort();
        switch(mess->getType())
        {
            case NoCMessageType::MEMORY_REQUEST_LOAD:
            case NoCMessageType::MEMORY_REQUEST_WB:
            case NoCMessageType::MEMORY_REQUEST_STORE:
            case NoCMessageType::MCPU_REQUEST:
                dst_id=dst_id+in_ports_tiles_.size();
                src_id=src_id+in_ports_tiles_.size();
                break;
            default:
                break;
        }
        logger_.logNoCMessageDestination(getClock()->currentCycle(), dst_id, 0); //mess->getRequest()->getPC());
        logger_.logNoCMessageSource(getClock()->currentCycle(), src_id, 0); //mess->getRequest()->getPC());
    }

    void NoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Packet counter for each network
        count_rx_packets_[mess->getNoCNetwork()]++;
        count_tx_packets_[mess->getNoCNetwork()]++;

        // Packet counter by each message type
        count_pkts_by_pkt_type[static_cast<int>(mess->getType())]++;
        switch(mess->getType())
        {
            // MEM -> VAS messages
            case NoCMessageType::MEMORY_ACK:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_COMMAND:
            // MEM -> MEM messages
            case NoCMessageType::MEM_TILE_REQUEST:
            case NoCMessageType::MEM_TILE_REPLY:
                break;
            default:
                sparta_assert(false, "Unsupported message received from a MCPU!!!");
        }
        
        if(trace_)
        {
            traceSrcDst_(mess);
        }
    }

} // spike_model
