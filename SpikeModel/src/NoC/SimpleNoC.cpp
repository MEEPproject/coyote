
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
        dst_count_(y_size_, vector<vector<uint64_t>>(x_size_, vector<uint64_t>(static_cast<int>(Networks::count), 0))),
        src_count_(y_size_, vector<vector<uint64_t>>(x_size_, vector<uint64_t>(static_cast<int>(Networks::count), 0))),
        dst_src_count_(y_size_, vector<vector<vector<vector<uint64_t>>>>(x_size_, vector<vector<vector<uint64_t>>>(
                   y_size_, vector<vector<uint64_t>>(x_size_, vector<uint64_t>(static_cast<int>(Networks::count), 0))))),
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
    }

    SimpleNoC::~SimpleNoC()
    {
        // Write the latest statistics file
        pkt_count_stage_--;
        writePacketCountMatrix_();
        debug_logger_ << getContainer()->getLocation() << ": " << std::endl;
    }

    void SimpleNoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill base statistics
        NoC::handleMessageFromTile_(mess);
        writePacketCountMatrix_();
        int hop_count = -1;
        sparta_assert(mess->getNoCNetwork() < static_cast<int>(Networks::count));
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
        switch(static_cast<Networks>(mess->getNoCNetwork()))
        {
            case Networks::DATA_TRANSFER_NOC:
                hop_count_data_transfer_ += hop_count;
                break;
            case Networks::ADDRESS_ONLY_NOC:
                hop_count_address_only_ += hop_count;
                break;
            case Networks::CONTROL_NOC:
                hop_count_control_ += hop_count;
                break;
            default:
                sparta_assert(false);
        }
    }

    void SimpleNoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill base statistics
        NoC::handleMessageFromMemoryCPU_(mess);
        writePacketCountMatrix_();
        int hop_count = -1;
        sparta_assert(mess->getNoCNetwork() < static_cast<int>(Networks::count));
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
        switch(static_cast<Networks>(mess->getNoCNetwork()))
        {
            case Networks::DATA_TRANSFER_NOC:
                hop_count_data_transfer_ += hop_count;
                break;
            case Networks::ADDRESS_ONLY_NOC:
                hop_count_address_only_ += hop_count;
                break;
            case Networks::CONTROL_NOC:
                hop_count_control_ += hop_count;
                break;
            default:
                sparta_assert(false);
        }
    }

    void SimpleNoC::writePacketCountMatrix_()
    {
        if(getClock()->currentCycle() < pkt_count_stage_ * pkt_count_period_)
            return;

        pkt_count_stage_++;

        std::ofstream dst_file_data_transfer;
        std::ofstream dst_file_address_only;
        std::ofstream dst_file_control;
        std::ofstream dst_file_aggregated;
        std::ofstream src_file_data_transfer;
        std::ofstream src_file_address_only;
        std::ofstream src_file_control;
        std::ofstream src_file_aggregated;
        std::ofstream dst_src_file_data_transfer;
        std::ofstream dst_src_file_address_only;
        std::ofstream dst_src_file_control;
        std::ofstream dst_src_file_aggregated;

        // DST files
        dst_file_data_transfer.open(pkt_count_prefix_ + "dst_datatransfer_" + std::to_string(getClock()->currentCycle()) + ".csv");
        dst_file_address_only.open(pkt_count_prefix_ + "dst_addressonly_" + std::to_string(getClock()->currentCycle()) + ".csv");
        dst_file_control.open(pkt_count_prefix_ + "dst_control_" + std::to_string(getClock()->currentCycle()) + ".csv");
        dst_file_aggregated.open(pkt_count_prefix_ + "dst_aggregated_" + std::to_string(getClock()->currentCycle()) + ".csv");
        uint64_t aggregated;
        for(int y=0; y < y_size_; ++y)
        {
            for(int x=0; x < x_size_; ++x)
            {
                aggregated = 0;
                dst_file_data_transfer << dst_count_[y][x][static_cast<int>(Networks::DATA_TRANSFER_NOC)] << ";";
                aggregated += dst_count_[y][x][static_cast<int>(Networks::DATA_TRANSFER_NOC)];
                dst_file_address_only << dst_count_[y][x][static_cast<int>(Networks::ADDRESS_ONLY_NOC)] << ";";
                aggregated += dst_count_[y][x][static_cast<int>(Networks::ADDRESS_ONLY_NOC)];
                dst_file_control << dst_count_[y][x][static_cast<int>(Networks::CONTROL_NOC)] << ";";
                aggregated += dst_count_[y][x][static_cast<int>(Networks::CONTROL_NOC)];
                dst_file_aggregated << aggregated << ";";
                if(pkt_count_flush_)
                {
                    dst_count_[y][x][static_cast<int>(Networks::DATA_TRANSFER_NOC)] = 0;
                    dst_count_[y][x][static_cast<int>(Networks::ADDRESS_ONLY_NOC)] = 0;
                    dst_count_[y][x][static_cast<int>(Networks::CONTROL_NOC)] = 0;
                }
                if(trace_)
                {
                    logger_.logNoCMessageDestinationCummulated(getClock()->currentCycle(), y*dst_count_[0].size()+x, 0, aggregated);
                }
            }
            dst_file_data_transfer << "\n";
            dst_file_address_only << "\n";
            dst_file_control << "\n";
            dst_file_aggregated << "\n";
        }
        dst_file_data_transfer.close();
        dst_file_address_only.close();
        dst_file_control.close();
        dst_file_aggregated.close();

        // SRC files
        src_file_data_transfer.open(pkt_count_prefix_ + "src_datatransfer_" + std::to_string(getClock()->currentCycle()) + ".csv");
        src_file_address_only.open(pkt_count_prefix_ + "src_addressonly_" + std::to_string(getClock()->currentCycle()) + ".csv");
        src_file_control.open(pkt_count_prefix_ + "src_control_" + std::to_string(getClock()->currentCycle()) + ".csv");
        src_file_aggregated.open(pkt_count_prefix_ + "src_aggregated_" + std::to_string(getClock()->currentCycle()) + ".csv");
        for(int y=0; y < y_size_; ++y)
        {
            for(int x=0; x < x_size_; ++x)
            {
                aggregated = 0;
                src_file_data_transfer << src_count_[y][x][static_cast<int>(Networks::DATA_TRANSFER_NOC)] << ";";
                aggregated += src_count_[y][x][static_cast<int>(Networks::DATA_TRANSFER_NOC)];
                src_file_address_only << src_count_[y][x][static_cast<int>(Networks::ADDRESS_ONLY_NOC)] << ";";
                aggregated += src_count_[y][x][static_cast<int>(Networks::ADDRESS_ONLY_NOC)];
                src_file_control << src_count_[y][x][static_cast<int>(Networks::CONTROL_NOC)] << ";";
                aggregated += src_count_[y][x][static_cast<int>(Networks::CONTROL_NOC)];
                src_file_aggregated << aggregated << ";";
                if(pkt_count_flush_)
                {
                    src_count_[y][x][static_cast<int>(Networks::DATA_TRANSFER_NOC)] = 0;
                    src_count_[y][x][static_cast<int>(Networks::ADDRESS_ONLY_NOC)] = 0;
                    src_count_[y][x][static_cast<int>(Networks::CONTROL_NOC)] = 0;
                }
                if(trace_)
                {
                    logger_.logNoCMessageSourceCummulated(getClock()->currentCycle(), y*dst_count_.size()+x, 0, aggregated);
                }
            }
            src_file_data_transfer << "\n";
            src_file_address_only << "\n";
            src_file_control << "\n";
            src_file_aggregated << "\n";
        }
        src_file_data_transfer.close();
        src_file_address_only.close();
        src_file_control.close();
        src_file_aggregated.close();

        // DST_SRC files
        dst_src_file_data_transfer.open(pkt_count_prefix_ + "dst_src_datatransfer_" + std::to_string(getClock()->currentCycle()) + ".csv");
        dst_src_file_address_only.open(pkt_count_prefix_ + "dst_src_addressonly_" + std::to_string(getClock()->currentCycle()) + ".csv");
        dst_src_file_control.open(pkt_count_prefix_ + "dst_src_control_" + std::to_string(getClock()->currentCycle()) + ".csv");
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
                        dst_src_file_data_transfer << dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::DATA_TRANSFER_NOC)] << ";";
                        aggregated += dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::DATA_TRANSFER_NOC)];
                        dst_src_file_address_only << dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::ADDRESS_ONLY_NOC)] << ";";
                        aggregated += dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::ADDRESS_ONLY_NOC)];
                        dst_src_file_control << dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::CONTROL_NOC)] << ";";
                        aggregated += dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::CONTROL_NOC)];
                        dst_src_file_aggregated << aggregated << ";";
                        if(pkt_count_flush_)
                        {
                            dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::DATA_TRANSFER_NOC)] = 0;
                            dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::ADDRESS_ONLY_NOC)] = 0;
                            dst_src_count_[dsty][dstx][srcy][srcx][static_cast<int>(Networks::CONTROL_NOC)] = 0;
                        }
                    }
                }
                dst_src_file_data_transfer << "\n";
                dst_src_file_address_only << "\n";
                dst_src_file_control << "\n";
                dst_src_file_aggregated << "\n";
            }
        }
        dst_src_file_data_transfer.close();
        dst_src_file_address_only.close();
        dst_src_file_control.close();
        dst_src_file_aggregated.close();
    }

} // spike_model
