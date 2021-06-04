#include <fstream>
#include <cmath>
#include "DetailedNoC.hpp"
#include "NoCMessage.hpp"
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
        stats_files_prefix_(params->stats_files_prefix),
        pkts_map_(vector(static_cast<int>(Networks::count), map<int,shared_ptr<NoCMessage>>()))
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
            sparta_assert(size_ == pow(k, n), "The network size must be the same in BookSim and in Coyote");
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
            sparta_assert(size_ == booksim_size, "The network size must be the same in BookSim and in Coyote");
        }
        else
            sparta_assert(false, "The supported networks are: mesh and cmesh");
        // Classes checks
        const uint8_t classes = booksim_config.GetInt("classes");
        uint8_t priorities = max_class_used_ + 1; // The number of priorities are: 0 .. max_class_used_
        sparta_assert(classes == priorities, 
            "The number of classes in BookSim config must be: " << std::to_string(priorities));
        // VCs checks
        const uint8_t num_vcs = booksim_config.GetInt("num_vcs");
        sparta_assert(num_vcs == 1 || num_vcs % priorities == 0, 
            "The num_vcs in BookSim config must be 1 or a multiple of: " << priorities);
        if(num_vcs > 1)
        {
            const vector<int> start_vc = booksim_config.GetIntArray("start_vc");
            const vector<int> end_vc = booksim_config.GetIntArray("end_vc");
            sparta_assert(start_vc.size() == priorities &&
                          end_vc.size() == priorities,
                          "The number of elements in start_vc and end_vc must be: " << priorities);
            sparta_assert(start_vc[0] == 0, "The first VC in start_vc must be 0");
            for(int i=1; i < priorities; ++i)
                sparta_assert(start_vc[i] == end_vc[i-1]+1, "The position i in start_vc must be equal to end_vc[i-1]+1");
            sparta_assert(end_vc[priorities-1] >= start_vc[priorities-1], "The latest value in end_vc must be >= than the latest value in start_vc");
            int count = priorities;
            for(int i=0; i < priorities; ++i)
                count += end_vc[i] - start_vc[i];
            sparta_assert(count == num_vcs, "The number of VCs calculated as end_vc-start_vc for the same class must be the same as the num_vcs");
        }

        // Get the injection queue size
        min_space_in_inj_queue_ = booksim_config.GetInt("injection_queue_size");

        // Fill the mcpu_ and tile_to_network and network_is_mcpu vectors
        uint16_t mcpu = 0;
        uint16_t tile = 0;
        for(uint16_t idx = 0; idx < size_; idx++)
        { // The validity of SRC and DST IDs (saved in mcpu_ and tile_to_network vectors) are enforced here because are >=0 and < size_
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

        // Create the wrappers for the NoC networks
        for(int n=0; n < static_cast<int>(Networks::count); ++n)
            booksim_wrappers_.push_back(new Booksim::BooksimWrapper(booksim_configuration_));
        sparta_assert(booksim_wrappers_.size() == static_cast<int>(Networks::count));
    }

    DetailedNoC::~DetailedNoC()
    {
        // Print booksim statistics at the end of the execution
        std::ofstream booksim_stats;
        for(int n=0; n < static_cast<int>(Networks::count); ++n)
        {
            string filename = stats_files_prefix_;
            switch(static_cast<Networks>(n))
            {
                case Networks::DATA_TRANSFER_NOC:
                    filename += "_datatransfer";
                    break;
                case Networks::ADDRESS_ONLY_NOC:
                    filename += "_addressonly";
                    break;
                case Networks::CONTROL_NOC:
                    filename += "_control";
                    break;
                default:
                    sparta_assert(false);
            }
            filename += ".txt";
            booksim_stats.open(filename);
            booksim_wrappers_[n]->PrintStats(booksim_stats);
            booksim_stats.close();
            // Check a missed in-flight packet
            sparta_assert(pkts_map_[n].size() == 0);
        }

        // Delete BookSim wrappers
        for(int i=static_cast<int>(Networks::count)-1; i >= 0; --i)
            delete booksim_wrappers_[i];
        debug_logger_ << getContainer()->getLocation() << ": " << std::endl;
    }

    void DetailedNoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the global statistics
        NoC::handleMessageFromTile_(mess);
        // Calculate and check size
        int size = (int) ceil(1.0*mess->getSize()/network_width_[mess->getNoCNetwork()]); // message size and network_width are in bits
        sparta_assert(size >= 1);
        int inj_queue_size = booksim_wrappers_[mess->getNoCNetwork()]->CheckInjectionQueue(tile_to_network_[mess->getSrcPort()], mess->getClass());
        sparta_assert(inj_queue_size >= size,
               "The injection queues are not well dimensioned, please review the injection_queue_size parameter.");
        // Save the min space available at injection queues
        if(inj_queue_size < (int) min_space_in_inj_queue_)
            min_space_in_inj_queue_ = inj_queue_size;
        int packet_id = INVALID_PKT_ID;
        switch(mess->getType())
        {
            // VAS -> VAS messages
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::REMOTE_L2_ACK:
                packet_id = booksim_wrappers_[mess->getNoCNetwork()]->GeneratePacket(
                    tile_to_network_[mess->getSrcPort()],   // Source
                    tile_to_network_[mess->getDstPort()],   // Destination
                    size,                                   // Number of flits
                    mess->getClass(),                       // Class of traffic -> Priority / VN / VC
                    INJECTION_TIME                          // Injection time to add
                );
                sparta_assert(packet_id != INVALID_PKT_ID);
                break;

            // VAS -> MCPU messages
            case NoCMessageType::MEMORY_REQUEST_LOAD:
            case NoCMessageType::MEMORY_REQUEST_STORE:
            case NoCMessageType::MEMORY_REQUEST_WB:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_ACK:
            case NoCMessageType::SCRATCHPAD_DATA_REPLY:
                packet_id = booksim_wrappers_[mess->getNoCNetwork()]->GeneratePacket(
                    tile_to_network_[mess->getSrcPort()],   // Source
                    mcpu_to_network_[mess->getDstPort()],   // Destination
                    size,                                   // Number of flits
                    mess->getClass(),                       // Class of traffic -> Priority / VN / VC
                    INJECTION_TIME                          // Injection time to add
                );
                sparta_assert(packet_id != INVALID_PKT_ID);
                break;

            default:
                sparta_assert(false);
        }
        pkts_map_[mess->getNoCNetwork()][packet_id] = mess;
        // Update sent flits for each network
        switch(static_cast<Networks>(mess->getNoCNetwork()))
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
        // Calculate and check size
        int size = (int) ceil(1.0*mess->getSize()/network_width_[mess->getNoCNetwork()]); // message size and network_width are in bits
        sparta_assert(size >= 1);
        int inj_queue_size = booksim_wrappers_[mess->getNoCNetwork()]->CheckInjectionQueue(mcpu_to_network_[mess->getSrcPort()], mess->getClass());
        sparta_assert(inj_queue_size >= size,
               "The injection queues are not well dimensioned, please review the injection_queue_size parameter.");
        // Save the min space available at injection queues
        if(inj_queue_size < (int) min_space_in_inj_queue_)
            min_space_in_inj_queue_ = inj_queue_size;
        int packet_id = INVALID_PKT_ID;
        switch(mess->getType())
        {
            // MCPU -> VAS messages
            case NoCMessageType::MEMORY_ACK:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_COMMAND:
                packet_id = booksim_wrappers_[mess->getNoCNetwork()]->GeneratePacket(
                    mcpu_to_network_[mess->getSrcPort()],   // Source
                    tile_to_network_[mess->getDstPort()],   // Destination
                    size,                                   // Number of flits
                    mess->getClass(),                       // Class of traffic -> Priority / VN / VC
                    INJECTION_TIME                          // Injection time to add
                );
                sparta_assert(packet_id != INVALID_PKT_ID);
                break;

            default:
                sparta_assert(false);
        }
        pkts_map_[mess->getNoCNetwork()][packet_id] = mess;
        // Update sent flits for each network
        switch(static_cast<Networks>(mess->getNoCNetwork()))
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
        vector<bool> run_booksim_at_next_cycle = vector(static_cast<int>(Networks::count), true); // Just after a retired packet or after be called with multiple cycles
        
        if(SPARTA_EXPECT_TRUE(cycles == 1)) // Run one cycle
        {
            for(int n=0; n < static_cast<int>(Networks::count); ++n)
            {
                Booksim::BooksimWrapper::RetiredPacket pkt;
                // Run BookSim
                booksim_wrappers_[n]->RunCycles(cycles);
                // Retire packet (if there)
                pkt = booksim_wrappers_[n]->RetirePacket();
                if(pkt.pid != INVALID_PKT_ID)
                {
                    // Get the message
                    std::shared_ptr<NoCMessage> mess = pkts_map_[n][pkt.pid];
                    sparta_assert(mess->getNoCNetwork() == n);
                    sparta_assert(mess->getClass() == pkt.c);
                    pkts_map_[n].erase(pkt.pid);
                    // Update statistics for each network
                    switch(static_cast<Networks>(n))
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
                    run_booksim_at_next_cycle[n] = booksim_wrappers_[n]->CheckInFlightPackets();
            }
        }
        else // Advance BookSim simulation time
        {
            for(int n=0; n < static_cast<int>(Networks::count); ++n)
            {
                sparta_assert(!booksim_wrappers_[n]->CheckInFlightPackets());
                sparta_assert(cycles);
                booksim_wrappers_[n]->UpdateSimTime(cycles);
            }
        }
        
        // Return if any NoC network needs to run BookSim at the next cycle
        bool any_network_needs_to_run_at_next_cycle = false;
        for(int n=0; n < static_cast<int>(Networks::count) && !any_network_needs_to_run_at_next_cycle; ++n)
            any_network_needs_to_run_at_next_cycle |= run_booksim_at_next_cycle[n];
        return any_network_needs_to_run_at_next_cycle;
    }

} // spike_model
