#include <fstream>
#include "DetailedNoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"

#define CLASS 0
#define INJECTION_TIME 0
#define INVALID_PKT_ID -1

namespace spike_model
{
    DetailedNoC::DetailedNoC(sparta::TreeNode *node, const DetailedNoCParameterSet *params) :
        NoC(node, params),
        booksim_configuration_(params->booksim_configuration),
        size_(num_tiles_ + num_memory_cpus_),
        network_width_(params->network_width),
        stats_file_(params->stats_file)
    {
        sparta_assert(noc_model_ == "detailed");
        sparta_assert((size_ & (size_ - 1)) == 0, "The size of the matrix must be power of two");
        sparta_assert(params->mcpus_indices.isVector(), "The top.cpu.noc.params.mcpus_indices must be a vector");
        sparta_assert(params->mcpus_indices.getNumValues() == num_memory_cpus_, 
            "The number of elements in mcpus_indices must be equal to the number of MCPUs");
        // Fill the mcpu_ and tile_to_network and network_is_mcpu vectors
        uint16_t mcpu = 0;
        uint16_t tile = 0;
        for(uint16_t idx = 0; idx < size_; idx++)
        {
            if(mcpu < params->mcpus_indices.getNumValues() && 
               params->mcpus_indices.getValueAsStringAt(mcpu) == std::to_string(idx))
            {
                mcpu_to_network_.push_back(idx);
                network_is_mcpu_.push_back(true);
                mcpu++;
            }
            else
            {
                tile_to_network_.push_back(idx);
                network_is_mcpu_.push_back(false);
                tile++;
            }
        }
        sparta_assert(mcpu_to_network_.size() == num_memory_cpus_);
        sparta_assert(tile_to_network_.size() == num_tiles_);
        booksim_wrapper_ = new Booksim::BooksimWrapper(booksim_configuration_);
    }

    DetailedNoC::~DetailedNoC()
    {
        // Print booksim statistics at the end of the execution
        std::ofstream booksim_stats;
        booksim_stats.open(stats_file_);
        booksim_wrapper_->PrintStats(booksim_stats);
        booksim_stats.close();

        delete booksim_wrapper_;
        debug_logger_ << getContainer()->getLocation() << ": " << std::endl;
        sparta_assert(pkts_map_.size() == 0); // Check a missed in-flight packet
    }

    void DetailedNoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the global statistics
        NoC::handleMessageFromTile_(mess);
        int size = (int) ceil(mess->getSize()*8.0/network_width_); // message size is in Bytes
        int packet_id = INVALID_PKT_ID;
        switch(mess->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::REMOTE_L2_ACK:
                while(packet_id == INVALID_PKT_ID)
                    packet_id = booksim_wrapper_->GeneratePacket(
                        tile_to_network_[mess->getSrcPort()],   // Source
                        tile_to_network_[mess->getDstPort()],   // Destination
                        size,                                   // Number of flits
                        CLASS,                                  // Class of traffic
                        INJECTION_TIME                          // Injection time to add
                    );
                break;

            case NoCMessageType::MEMORY_REQUEST:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_ACK:
                while(packet_id == INVALID_PKT_ID)
                    packet_id = booksim_wrapper_->GeneratePacket(
                        tile_to_network_[mess->getSrcPort()],   // Source
                        mcpu_to_network_[mess->getDstPort()],   // Destination
                        size,                                   // Number of flits
                        CLASS,                                  // Class of traffic
                        INJECTION_TIME                          // Injection time to add
                    );
                break;

            default:
                sparta_assert(false);
        }
        pkts_map_[packet_id] = mess;
        count_tx_flits_ += size;
    }

    void DetailedNoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the global statistics
        NoC::handleMessageFromMemoryCPU_(mess);
        int size = (int) ceil(mess->getSize()*8.0/network_width_); // message size is in Bytes
        int packet_id = INVALID_PKT_ID;
        switch(mess->getType())
        {
            case NoCMessageType::MEMORY_ACK:
            case NoCMessageType::MCPU_REQUEST:
                while(packet_id == INVALID_PKT_ID)
                    packet_id = booksim_wrapper_->GeneratePacket(
                        mcpu_to_network_[mess->getSrcPort()],   // Source
                        tile_to_network_[mess->getDstPort()],   // Destination
                        size,                                   // Number of flits
                        CLASS,                                  // Class of traffic
                        INJECTION_TIME                          // Injection time to add
                    );
                break;

            default:
                sparta_assert(false);
        }
        pkts_map_[packet_id] = mess;
        count_tx_flits_ += size;
    }

    bool DetailedNoC::runBookSimCycles(const uint16_t cycles, const uint64_t current_cycle)
    {
        bool run_booksim_at_next_cycle = true; // Just after a retired packet or after be called with multiple cycles
        
        if(SPARTA_EXPECT_TRUE(cycles == 1)) // Run one cycle
        {
            Booksim::BooksimWrapper::RetiredPacket pkt;
            // Run BookSim
            booksim_wrapper_->RunCycles(cycles);
            // Retire packet (if there)
            pkt = booksim_wrapper_->RetirePacket();
            if(pkt.pid != INVALID_PKT_ID)
            {
                // Get the message
                std::shared_ptr<NoCMessage> mess = pkts_map_[pkt.pid];
                pkts_map_.erase(pkt.pid);
                // Update statistics
                hop_count_ += pkt.hops;
                count_rx_flits_ += pkt.ps;
                packet_latency_ += pkt.plat;
                network_latency_ += pkt.nlat;
                // Send to the actual destination at NEXT_CYCLE
                int rel_time = current_cycle + 1 - getClock()->currentCycle(); // rel_time to NEXT_CYCLE = current+1
                sparta_assert(rel_time >= 0); // rel_time must be a cycle that sparta can run after current booksim iteration
                if(network_is_mcpu_[pkt.dst])
                    out_ports_memory_cpus_[mess->getDstPort()]->send(mess, rel_time); // to MCPU
                else
                    out_ports_tiles_[mess->getDstPort()]->send(mess, rel_time);       // to TILE
            } 
            else // If there is no retired packet, BookSim may not have an event on the next cycle
                run_booksim_at_next_cycle = booksim_wrapper_->CheckInFlightPackets();
        }
        else // Advance BookSim simulation time
        {
            sparta_assert(!booksim_wrapper_->CheckInFlightPackets());
            sparta_assert(cycles);
            booksim_wrapper_->UpdateSimTime(cycles);
        }

        return run_booksim_at_next_cycle;
    }

} // spike_model
