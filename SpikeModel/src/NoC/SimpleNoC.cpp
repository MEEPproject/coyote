
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

namespace spike_model
{
    SimpleNoC::SimpleNoC(sparta::TreeNode *node, const SimpleNoCParameterSet *params) :
        NoC(node, params),
        latency_per_hop_(params->latency_per_hop),
        x_size_(params->x_size),
        y_size_(params->y_size),
        dst_count_(x_size_, std::vector<uint64_t>(y_size_, 0)),
        src_count_(x_size_, std::vector<uint64_t>(y_size_, 0)),
        pkt_count_prefix_(params->packet_count_file_prefix),
        pkt_count_period_(params->packet_count_periodicity),
        pkt_count_stage_(0)
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
        switch(mess->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::REMOTE_L2_ACK:
                hop_count = abs(tiles_coordinates_[mess->getDstPort()].first - tiles_coordinates_[mess->getSrcPort()].first) + 
                            abs(tiles_coordinates_[mess->getDstPort()].second - tiles_coordinates_[mess->getSrcPort()].second) +
                            DESTINATION_ROUTER;
                hop_count_ += hop_count;
                dst_count_[tiles_coordinates_[mess->getDstPort()].first][tiles_coordinates_[mess->getDstPort()].second]++;
                src_count_[tiles_coordinates_[mess->getSrcPort()].first][tiles_coordinates_[mess->getSrcPort()].second]++;
                // Latency: Injection + Link traversal + hops * latency_per_hop (RC - VA - SA - ST + output_link)
                out_ports_tiles_[mess->getDstPort()]->send(mess, INJECTION + LINK_TRAVERSAL + hop_count*latency_per_hop_);
                break;

            case NoCMessageType::MEMORY_REQUEST:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_ACK:
                hop_count = abs(mcpus_coordinates_[mess->getDstPort()].first - tiles_coordinates_[mess->getSrcPort()].first) + 
                            abs(mcpus_coordinates_[mess->getDstPort()].second - tiles_coordinates_[mess->getSrcPort()].second) + 
                            DESTINATION_ROUTER;
                hop_count_ += hop_count;
                dst_count_[mcpus_coordinates_[mess->getDstPort()].first][mcpus_coordinates_[mess->getDstPort()].second]++;
                src_count_[tiles_coordinates_[mess->getSrcPort()].first][tiles_coordinates_[mess->getSrcPort()].second]++;
                out_ports_memory_cpus_[mess->getDstPort()]->send(mess, INJECTION + LINK_TRAVERSAL + hop_count*latency_per_hop_);
                break;

            default:
                sparta_assert(false);
        }
        sparta_assert(hop_count >= 0 && hop_count <= x_size_ + y_size_ - 1);
    }

    void SimpleNoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill base statistics
        NoC::handleMessageFromMemoryCPU_(mess);
        writePacketCountMatrix_();
        int hop_count = -1;
        switch(mess->getType())
        {
            case NoCMessageType::MEMORY_ACK:
            case NoCMessageType::MCPU_REQUEST:
                hop_count = abs(tiles_coordinates_[mess->getDstPort()].first - mcpus_coordinates_[mess->getSrcPort()].first) + 
                            abs(tiles_coordinates_[mess->getDstPort()].second - mcpus_coordinates_[mess->getSrcPort()].second) +
                            DESTINATION_ROUTER;;
                hop_count_ += hop_count;
                dst_count_[tiles_coordinates_[mess->getDstPort()].first][tiles_coordinates_[mess->getDstPort()].second]++;
                src_count_[mcpus_coordinates_[mess->getSrcPort()].first][mcpus_coordinates_[mess->getSrcPort()].second]++;
                out_ports_tiles_[mess->getDstPort()]->send(mess, INJECTION + LINK_TRAVERSAL + hop_count*latency_per_hop_);
                break;

            default:
                sparta_assert(false);
        }
        sparta_assert(hop_count >= 0 && hop_count <= x_size_ + y_size_ - 1);
    }

    void SimpleNoC::writePacketCountMatrix_()
    {
        if(getClock()->currentCycle() < pkt_count_stage_ * pkt_count_period_)
            return;

        pkt_count_stage_++;

        std::ofstream dst_file;
        std::ofstream src_file;

        dst_file.open(pkt_count_prefix_ + "dst_" + std::to_string(getClock()->currentCycle()) + ".csv");
        for(int y=0; y < y_size_; ++y)
        {
            for(int x=0; x < x_size_; ++x)
                dst_file << dst_count_[x][y] << ";";
            dst_file << "\n";
        }
        dst_file.close();

        src_file.open(pkt_count_prefix_ + "src_" + std::to_string(getClock()->currentCycle()) + ".csv");
        for(int y=0; y < y_size_; ++y)
        {
            for(int x=0; x < x_size_; ++x)
                src_file << src_count_[x][y] << ";";
            src_file << "\n";
        }
        src_file.close();
    }

} // spike_model
