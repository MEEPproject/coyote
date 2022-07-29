// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 


#include "NoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryTile/MemoryCPUWrapper.hpp"
#include <chrono>

namespace coyote
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

        for(auto mess_header_size : params->message_header_size){
            int name_length = mess_header_size.find(":");
            int size = stoi(mess_header_size.substr(name_length+1));
            sparta_assert(size < std::numeric_limits<uint8_t>::max(),
                            "The header size should be lower than " + std::to_string(std::numeric_limits<uint8_t>::max()));
            NoCMessage::header_size[static_cast<int>(getMessageTypeFromString_(mess_header_size.substr(0,name_length)))] = size;
        }

        // Define the mapping of NoC Messages to Networks and classes
        sparta_assert(params->message_to_network_and_class.getNumValues() == static_cast<int>(NoCMessageType::count),
                        "The number of messages defined in message_to_network_and_class param is not correct.");

        for(auto mess_net_class : params->message_to_network_and_class){
            int mess_length = mess_net_class.find(":");
            int dot_pos = mess_net_class.find(".");
            int network_length = dot_pos - (mess_length+1);
            int class_value = stoi(mess_net_class.substr(dot_pos+1));
            sparta_assert(class_value >= 0 && class_value < std::numeric_limits<uint8_t>::max());
            if(class_value > max_class_used_)
                max_class_used_ = class_value;
            NoC::message_to_network_and_class_[getMessageTypeFromString_(mess_net_class.substr(0,mess_length))] = 
                std::make_pair(getNetworkFromString(mess_net_class.substr(mess_length+1,network_length)), class_value);
        }
        sparta_assert(NoC::message_to_network_and_class_.size() == static_cast<int>(NoCMessageType::count));

        // Packets queue
        vas_queue_.resize(noc_networks_.size());
        mem_queue_.resize(noc_networks_.size());
        for (uint8_t noc = 0; noc < noc_networks_.size(); noc++)
        {
            vas_queue_.at(noc).resize(num_tiles_);
            mem_queue_.at(noc).resize(num_memory_cpus_);
        }

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
    const std::string NoC::getNetworkName(const uint8_t noc) {return noc_networks_[noc];}

    void NoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        sparta_assert(mess->getNoCNetwork() < noc_networks_.size());
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

    bool NoC::deliverOnePacketToDestination(const uint64_t current_cycle)
    {
        vector<bool> run_noc_at_next_cycle = vector(noc_networks_.size(), false);
        // Like in detailed model we have a packet latency of X that does not include ejection latency and then, we eject the packets always 1 cycle later.
        // Hence, here it is ejected the packet at NEXT cycle (current + 1) to always use the concept of packet latency in the same way
        int rel_time = current_cycle + 1 - getClock()->currentCycle(); // getClock()->currentCycle() points to latest cycle + 1
        for (uint8_t noc = 0; noc < noc_networks_.size(); ++noc)
        {
            // vas
            for(uint16_t vas = 0; vas < num_tiles_; vas++)
            {
                // check if there is a packet to this vas tile
                if(!vas_queue_.at(noc).at(vas).empty() && vas_queue_.at(noc).at(vas).front().second <= current_cycle)
                {
                    std::shared_ptr<NoCMessage> mess = vas_queue_.at(noc).at(vas).front().first;
                    // Send to the actual destination at current + packet_latency_
                    out_ports_tiles_[vas]->send(mess, rel_time);
                    vas_queue_.at(noc).at(vas).erase(vas_queue_.at(noc).at(vas).begin());
                }
                // check if the are packets to be extracted at next cycle
                if(!run_noc_at_next_cycle[noc] && !vas_queue_.at(noc).at(vas).empty())
                    run_noc_at_next_cycle[noc] = true;
            }
            // mem
            for(uint16_t mem = 0; mem < num_memory_cpus_; mem++)
            {
                // check if there is a packet to this mem tile
                if(!mem_queue_.at(noc).at(mem).empty() && mem_queue_.at(noc).at(mem).front().second <= current_cycle)
                {
                    std::shared_ptr<NoCMessage> mess = mem_queue_.at(noc).at(mem).front().first;
                    sparta_assert(mem == mess->getDstPort());
                    // check if memory tile is able to receive the packet
                    // Memory tiles have the "magical" capability of being able to check if its next packet can be received
                    // by analyzing it without actually receiving it
                    if(!(*memoryTiles)[mem]->ableToReceivePacket(mess))
                    {
                        run_noc_at_next_cycle[noc] = true;
                        continue; // continue if MT is not able to receive the packet
                    }
                    // Send to the actual destination at current + packet_latency_
                    out_ports_memory_cpus_[mem]->send(mess, rel_time);
                    mem_queue_.at(noc).at(mem).erase(mem_queue_.at(noc).at(mem).begin());
                }
                // check if the are packets to be extracted at next cycle
                if(!run_noc_at_next_cycle[noc] && !mem_queue_.at(noc).at(mem).empty())
                    run_noc_at_next_cycle[noc] = true;
            }
        }
        // Return if any NoC network needs to be executed at the next cycle
        bool any_network_needs_to_run_at_next_cycle = false;
        for(uint8_t noc=0; noc < noc_networks_.size() && !any_network_needs_to_run_at_next_cycle; ++noc)
            any_network_needs_to_run_at_next_cycle |= run_noc_at_next_cycle[noc];

        return any_network_needs_to_run_at_next_cycle;
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
        logger_->logNoCMessageDestination(getClock()->currentCycle(), mess->getRequest()->getCoreId() , dst_id, mess->getRequest()->getPC());
        logger_->logNoCMessageSource(getClock()->currentCycle(), mess->getRequest()->getCoreId(), src_id, mess->getRequest()->getPC());
    }

    void NoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        sparta_assert(mess->getNoCNetwork() < noc_networks_.size());
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

    void NoC::setMemoryTiles(std::shared_ptr<std::vector<MemoryCPUWrapper *>> &newMemoryTiles) {
        memoryTiles = newMemoryTiles;
    }

} // coyote
