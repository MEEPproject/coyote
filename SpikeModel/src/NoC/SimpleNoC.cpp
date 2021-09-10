
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>
#include "SimpleNoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"

#define DESTINATION_ROUTER 1
#define INJECTION 1
#define LINK_TRAVERSAL 1

using std::abs;
using std::vector;

namespace spike_model
{
    SimpleNoC::SimpleNoC(sparta::TreeNode *node, const SimpleNoCParameterSet *params) :
        NoC(node, params),
        latency_per_hop_(params->latency_per_hop),
        x_size_(params->x_size),
        y_size_(params->y_size),
        dst_count_(y_size_, vector<vector<uint64_t>>(x_size_, vector<uint64_t>(noc_networks_.size(), 0))),
        src_count_(y_size_, vector<vector<uint64_t>>(x_size_, vector<uint64_t>(noc_networks_.size(), 0))),
        dst_src_count_(y_size_, vector<vector<vector<vector<uint64_t>>>>(x_size_, vector<vector<vector<uint64_t>>>(
                   y_size_, vector<vector<uint64_t>>(x_size_, vector<uint64_t>(noc_networks_.size(), 0))))),
        pkt_count_prefix_(params->packet_count_file_prefix),
        pkt_count_period_(params->packet_count_periodicity),
        pkt_count_stage_(0),
        pkt_count_flush_(params->flush_packet_count_each_period)
    {
        sparta_assert(noc_model_ == "simple");
        sparta_assert(x_size_ * y_size_ == num_memory_cpus_ + num_tiles_, 
            "The NoC mesh size is equal to the number of elements to connect:" << 
            "\n X: " << x_size_ <<
            "\n Y: " << y_size_ <<
            "\n PEs: " << num_memory_cpus_ + num_tiles_);
        sparta_assert(params->mcpus_location.isVector(), "The top.cpu.noc.params.mcpus_location must be a vector");
        sparta_assert(params->mcpus_location.getNumValues() == num_memory_cpus_, 
            "The number of elements in mcpus_location must be equal to the number of MCPUs");
        // Add all coordinates to tile_coordinates_ (first = X, second = Y)
        // Tiles are filled in order starting from (0,0) up to (x_size,y_size) without MCPUs
        for(int y=0; y < y_size_; ++y)
            for(int x=0; x < x_size_; ++x)
                tiles_coordinates_.push_back(std::make_pair(x, y));
        // Fill mcpus_coordinates_ vector and remove MCPUs from tiles_coordinates_
        int dot_pos;
        std::pair<uint16_t, uint16_t> current;
        for(auto mcpu_xy : params->mcpus_location){
            dot_pos = mcpu_xy.find(".");
            current = std::make_pair(stoi(mcpu_xy.substr(0,dot_pos)), stoi(mcpu_xy.substr(dot_pos+1)));
            mcpus_coordinates_.push_back(current);
            tiles_coordinates_.erase(std::remove(tiles_coordinates_.begin(), tiles_coordinates_.end(), current), tiles_coordinates_.end());
        }
        sparta_assert(mcpus_coordinates_.size() == num_memory_cpus_);
        sparta_assert(tiles_coordinates_.size() == num_tiles_);

        // Statistics
        for(auto network : noc_networks_)
        {
            hop_count_.push_back(sparta::Counter(
                getStatisticSet(),                                       // parent
                "hop_count_" + network,                                  // name
                "Total number of packet hops in " + network + " NoC",    // description
                sparta::Counter::COUNT_NORMAL                            // behavior
            ));

            average_hop_count_.push_back(sparta::StatisticDef(
                getStatisticSet(),                                      // parent
                "average_hop_count_" + network,                         // name
                "Average hop count in  " + network + " NoC",            // description
                getStatisticSet(),                                      // context
                "hop_count_" + network + "/sent_packets_"+network       // Expression
            ));
        }
    }

    SimpleNoC::~SimpleNoC()
    {
        // Write the latest statistics file
        pkt_count_stage_--;
        writePacketCountMatrix_();
        debug_logger_ << getContainer()->getLocation() << ": " << std::endl;
    }

    bool SimpleNoC::checkSpaceForPacket(const bool injectedByTile, const std::shared_ptr<NoCMessage> & mess){return true;}

    void SimpleNoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill base statistics
        NoC::handleMessageFromTile_(mess);
        writePacketCountMatrix_();
        int hop_count = -1;
        sparta_assert(mess->getNoCNetwork() < noc_networks_.size());
        switch(mess->getType())
        {
            // VAS -> VAS messages
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::REMOTE_L2_ACK:
                hop_count = abs(tiles_coordinates_[mess->getDstPort()].first - tiles_coordinates_[mess->getSrcPort()].first) + 
                            abs(tiles_coordinates_[mess->getDstPort()].second - tiles_coordinates_[mess->getSrcPort()].second) +
                            DESTINATION_ROUTER;
                dst_count_[tiles_coordinates_[mess->getDstPort()].second][tiles_coordinates_[mess->getDstPort()].first][mess->getNoCNetwork()]++; // [y][x][NoC]
                src_count_[tiles_coordinates_[mess->getSrcPort()].second][tiles_coordinates_[mess->getSrcPort()].first][mess->getNoCNetwork()]++;
                dst_src_count_[tiles_coordinates_[mess->getDstPort()].second][tiles_coordinates_[mess->getDstPort()].first][tiles_coordinates_[mess->getSrcPort()].second][tiles_coordinates_[mess->getSrcPort()].first][mess->getNoCNetwork()]++; //dst[y][x]src[y][x][NoC]
                // Latency: Injection + Link traversal + hops * latency_per_hop (RC - VA - SA - ST + output_link)
                out_ports_tiles_[mess->getDstPort()]->send(mess, INJECTION + LINK_TRAVERSAL + hop_count*latency_per_hop_);
                break;

            // VAS -> MCPU messages
            case NoCMessageType::MEMORY_REQUEST_LOAD:
            case NoCMessageType::MEMORY_REQUEST_STORE:
            case NoCMessageType::MEMORY_REQUEST_WB:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_ACK:
            case NoCMessageType::SCRATCHPAD_DATA_REPLY:
                hop_count = abs(mcpus_coordinates_[mess->getDstPort()].first - tiles_coordinates_[mess->getSrcPort()].first) + 
                            abs(mcpus_coordinates_[mess->getDstPort()].second - tiles_coordinates_[mess->getSrcPort()].second) + 
                            DESTINATION_ROUTER;
                dst_count_[mcpus_coordinates_[mess->getDstPort()].second][mcpus_coordinates_[mess->getDstPort()].first][mess->getNoCNetwork()]++; // [y][x][NoC]
                src_count_[tiles_coordinates_[mess->getSrcPort()].second][tiles_coordinates_[mess->getSrcPort()].first][mess->getNoCNetwork()]++;
                dst_src_count_[mcpus_coordinates_[mess->getDstPort()].second][mcpus_coordinates_[mess->getDstPort()].first][tiles_coordinates_[mess->getSrcPort()].second][tiles_coordinates_[mess->getSrcPort()].first][mess->getNoCNetwork()]++; //dst[y][x]src[y][x][NoC]
                out_ports_memory_cpus_[mess->getDstPort()]->send(mess, INJECTION + LINK_TRAVERSAL + hop_count*latency_per_hop_);
                break;

            default:
                sparta_assert(false);
        }
        sparta_assert(hop_count >= 0 && hop_count <= x_size_ + y_size_ - 1);
        // Hop count for each network
        hop_count_[mess->getNoCNetwork()] += hop_count;
    }

    void SimpleNoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill base statistics
        NoC::handleMessageFromMemoryCPU_(mess);
        writePacketCountMatrix_();
        int hop_count = -1;
        sparta_assert(mess->getNoCNetwork() < noc_networks_.size());
        switch(mess->getType())
        {
            // MCPU -> VAS messages
            case NoCMessageType::MEMORY_ACK:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_COMMAND:
                hop_count = abs(tiles_coordinates_[mess->getDstPort()].first - mcpus_coordinates_[mess->getSrcPort()].first) + 
                            abs(tiles_coordinates_[mess->getDstPort()].second - mcpus_coordinates_[mess->getSrcPort()].second) +
                            DESTINATION_ROUTER;;
                dst_count_[tiles_coordinates_[mess->getDstPort()].second][tiles_coordinates_[mess->getDstPort()].first][mess->getNoCNetwork()]++; // [y][x][NoC]
                src_count_[mcpus_coordinates_[mess->getSrcPort()].second][mcpus_coordinates_[mess->getSrcPort()].first][mess->getNoCNetwork()]++;
                dst_src_count_[tiles_coordinates_[mess->getDstPort()].second][tiles_coordinates_[mess->getDstPort()].first][mcpus_coordinates_[mess->getSrcPort()].second][mcpus_coordinates_[mess->getSrcPort()].first][mess->getNoCNetwork()]++; //dst[y][x]src[y][x][NoC]
                out_ports_tiles_[mess->getDstPort()]->send(mess, INJECTION + LINK_TRAVERSAL + hop_count*latency_per_hop_);
                break;

            default:
                sparta_assert(false);
        }
        sparta_assert(hop_count >= 0 && hop_count <= x_size_ + y_size_ - 1);
        // Hop count for each network
        hop_count_[mess->getNoCNetwork()] += hop_count;
    }

    void SimpleNoC::writePacketCountMatrix_()
    {
        if(getClock()->currentCycle() < pkt_count_stage_ * pkt_count_period_)
            return;

        pkt_count_stage_++;

        std::vector<std::ofstream> dst_file;
        std::ofstream dst_file_aggregated;
        std::vector<std::ofstream> src_file;
        std::ofstream src_file_aggregated;
        std::vector<std::ofstream> dst_src_file;
        std::ofstream dst_src_file_aggregated;

        dst_file.resize(noc_networks_.size());
        src_file.resize(noc_networks_.size());
        dst_src_file.resize(noc_networks_.size());

        // DST files
        for(auto network : noc_networks_)
        {
            dst_file[getNetworkFromString(network)].open(pkt_count_prefix_ + "dst_" + network + "_" + std::to_string(getClock()->currentCycle()) + ".csv");
        }
        dst_file_aggregated.open(pkt_count_prefix_ + "dst_aggregated_" + std::to_string(getClock()->currentCycle()) + ".csv");
        uint64_t aggregated;
        for(int y=0; y < y_size_; ++y)
        {
            for(int x=0; x < x_size_; ++x)
            {
                aggregated = 0;
                for(auto network : noc_networks_)
                {
                    dst_file[getNetworkFromString(network)] << dst_count_[y][x][getNetworkFromString(network)] << ";";
                    aggregated += dst_count_[y][x][getNetworkFromString(network)];
                    if(pkt_count_flush_)
                    {
                        dst_count_[y][x][getNetworkFromString(network)] = 0;
                    }
                }
                dst_file_aggregated << aggregated << ";";
                if(trace_)
                {
                    logger_.logNoCMessageDestinationCummulated(getClock()->currentCycle(), y*dst_count_[0].size()+x, 0, aggregated);
                }
            }
            for(auto network : noc_networks_)
                dst_file[getNetworkFromString(network)] << "\n";
            dst_file_aggregated << "\n";
        }
        for(auto network : noc_networks_)
        {
            dst_file[getNetworkFromString(network)].close();
        }
        dst_file_aggregated.close();

        // SRC files
        for(auto network : noc_networks_)
        {
            src_file[getNetworkFromString(network)].open(pkt_count_prefix_ + "src_" + network + "_" + std::to_string(getClock()->currentCycle()) + ".csv");
        }
        src_file_aggregated.open(pkt_count_prefix_ + "src_aggregated_" + std::to_string(getClock()->currentCycle()) + ".csv");
        for(int y=0; y < y_size_; ++y)
        {
            for(int x=0; x < x_size_; ++x)
            {
                aggregated = 0;
                for(auto network : noc_networks_)
                {
                    src_file[getNetworkFromString(network)] << src_count_[y][x][getNetworkFromString(network)] << ";";
                    aggregated += src_count_[y][x][getNetworkFromString(network)];
                    if(pkt_count_flush_)
                    {
                        src_count_[y][x][getNetworkFromString(network)] = 0;
                    }
                }
                src_file_aggregated << aggregated << ";";
                if(trace_)
                {
                    logger_.logNoCMessageSourceCummulated(getClock()->currentCycle(), y*dst_count_.size()+x, 0, aggregated);
                }
            }
            for(auto network : noc_networks_)
            {
                src_file[getNetworkFromString(network)] << "\n";
            }
            src_file_aggregated << "\n";
        }
        for(auto network : noc_networks_)
        {
            src_file[getNetworkFromString(network)].close();
        }
        src_file_aggregated.close();

        // DST_SRC files
        for(auto network : noc_networks_)
        {
            dst_src_file[getNetworkFromString(network)].open(pkt_count_prefix_ + "dst_src_" + network + "_" + std::to_string(getClock()->currentCycle()) + ".csv");
        }
        dst_src_file_aggregated.open(pkt_count_prefix_ + "dst_src_aggregated_" + std::to_string(getClock()->currentCycle()) + ".csv");
        for(int dsty=0; dsty < y_size_; ++dsty)
        {
            for(int dstx=0; dstx < x_size_; ++dstx)
            {
                for(int srcy=0; srcy < y_size_; ++srcy)
                {
                    for(int srcx=0; srcx < x_size_; ++srcx)
                    {
                        aggregated = 0;
                        for(auto network : noc_networks_)
                        {
                            dst_src_file[getNetworkFromString(network)] << dst_src_count_[dsty][dstx][srcy][srcx][getNetworkFromString(network)] << ";";
                            aggregated += dst_src_count_[dsty][dstx][srcy][srcx][getNetworkFromString(network)];
                            if(pkt_count_flush_)
                            {
                                dst_src_count_[dsty][dstx][srcy][srcx][getNetworkFromString(network)] = 0;
                            }
                        }
                        dst_src_file_aggregated << aggregated << ";";
                    }
                }
                for(auto network : noc_networks_)
                {
                    dst_src_file[getNetworkFromString(network)] << "\n";
                }
                dst_src_file_aggregated << "\n";
            }
        }
        for(auto network : noc_networks_)
        {
            dst_src_file[getNetworkFromString(network)].close();
        }
        dst_src_file_aggregated.close();
    }

} // spike_model
