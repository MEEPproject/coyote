#include <fstream>
#include <cmath>
#include "DetailedNoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"

#include "booksim_config.hpp"

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
        sparta_assert(params->mcpus_indices.isVector(), "The top.cpu.noc.params.mcpus_indices must be a vector");
        sparta_assert(params->mcpus_indices.getNumValues() == num_memory_cpus_, 
            "The number of elements in mcpus_indices must be equal to the number of MCPUs");
        sparta_assert(params->network_width.isVector(), "The top.cpu.noc.params.network_width must be a vector");
        sparta_assert(params->network_width.getNumValues() == static_cast<int>(Networks::count),
            "The number of elements in top.cpu.noc.params.network_width must be: " << static_cast<int>(Networks::count));

        // Parse BookSim configuration to extract information and checks
        Booksim::BookSimConfig booksim_config;
        booksim_config.ParseFile(booksim_configuration_);
        const string topology = booksim_config.GetStr("topology");
        if(topology == "mesh") // Implemented as kncube network in BookSim
        {
            int n = booksim_config.GetInt("n");
            int k = booksim_config.GetInt("k");
            sparta_assert(size_ == pow(k, n), "The network size must be the same in BookSim and Coyote");
        } 
        else if(topology == "cmesh") // Implemented as ckncube network in BookSim
        {
            int n = booksim_config.GetInt("n");
            vector<int> k = booksim_config.GetIntArray("k");
            vector<int> c = booksim_config.GetIntArray("c");
            int booksim_size = 1;
            for(int dim=0; dim < n; ++dim){
                sparta_assert(c[dim] == 1, "The concentration must be equal to 1");
                booksim_size *= k[dim];
            }
            sparta_assert(size_ == booksim_size, "The network size must be the same in BookSim and Coyote");
        }
        else
            sparta_assert(false, "The supported networks are: mesh and cmesh");
        // Classes checks
        const int classes = booksim_config.GetInt("classes");
        sparta_assert(classes == static_cast<int>(Networks::count), 
            "The number of classes in BookSim config must be: " << static_cast<int>(Networks::count));
        const int num_vcs = booksim_config.GetInt("num_vcs");
        sparta_assert(num_vcs == static_cast<int>(Networks::count), 
            "The number of num_vcs in BookSim config must be: " << static_cast<int>(Networks::count));
        const vector<int> start_vc = booksim_config.GetIntArray("start_vc");
        const vector<int> end_vc = booksim_config.GetIntArray("end_vc");
        sparta_assert(start_vc.size() == static_cast<int>(Networks::count) &&
                      end_vc.size() == static_cast<int>(Networks::count),
                      "The number of elements in start_vc and end_vc must be: " << static_cast<int>(Networks::count));

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
        int size = (int) ceil(1.0*mess->getSize()/network_width_[mess->getTransactionType()]); // message size and network_width are in bits
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
                        mess->getTransactionType(),             // Class of traffic -> Network -> Represented as VC
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
                        mess->getTransactionType(),             // Class of traffic -> Network -> Represented as VC
                        INJECTION_TIME                          // Injection time to add
                    );
                break;

            default:
                sparta_assert(false);
        }
        pkts_map_[packet_id] = mess;
        // Update sent flits for each network
        switch(static_cast<Networks>(mess->getTransactionType()))
        {
            case Networks::DATA_TRANSFER_NOC:
                count_tx_flits_data_transfer_ += size;
                break;
            case Networks::ADDRESS_ONLY_NOC:
                count_tx_flits_address_only_ += size;
                break;
            case Networks::CONTROL_NOC:
                count_tx_flits_control_ += size;
                break;
            default:
                sparta_assert(false);
        }
    }

    void DetailedNoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the global statistics
        NoC::handleMessageFromMemoryCPU_(mess);
        int size = (int) ceil(1.0*mess->getSize()/network_width_[mess->getTransactionType()]); // message size and network_width are in bits
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
                        mess->getTransactionType(),             // Class of traffic -> Network -> Represented as VC
                        INJECTION_TIME                          // Injection time to add
                    );
                break;

            default:
                sparta_assert(false);
        }
        pkts_map_[packet_id] = mess;
        // Update sent flits for each network
        switch(static_cast<Networks>(mess->getTransactionType()))
        {
            case Networks::DATA_TRANSFER_NOC:
                count_tx_flits_data_transfer_ += size;
                break;
            case Networks::ADDRESS_ONLY_NOC:
                count_tx_flits_address_only_ += size;
                break;
            case Networks::CONTROL_NOC:
                count_tx_flits_control_ += size;
                break;
            default:
                sparta_assert(false);
        }
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
                // Update statistics for each network
                switch(static_cast<Networks>(mess->getTransactionType()))
                {
                    case Networks::DATA_TRANSFER_NOC:
                        hop_count_data_transfer_ += pkt.hops;
                        count_rx_flits_data_transfer_ += pkt.ps;
                        packet_latency_data_transfer_ += pkt.plat;
                        network_latency_data_transfer_ += pkt.nlat;
                        break;
                    case Networks::ADDRESS_ONLY_NOC:
                        hop_count_address_only_ += pkt.hops;
                        count_rx_flits_address_only_ += pkt.ps;
                        packet_latency_address_only_ += pkt.plat;
                        network_latency_address_only_ += pkt.nlat;
                        break;
                    case Networks::CONTROL_NOC:
                        hop_count_control_ += pkt.hops;
                        count_rx_flits_control_ += pkt.ps;
                        packet_latency_control_ += pkt.plat;
                        network_latency_control_ += pkt.nlat;
                        break;
                    default:
                        sparta_assert(false);
                }
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
