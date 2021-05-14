
#ifndef __DETAILED_NOC_H__
#define __DETAILED_NOC_H__

#include "NoC.hpp"
#include "booksim_wrapper.hpp"

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;

namespace spike_model
{
    class DetailedNoC : public NoC
    {
        /*!
         * \class spike_model::DetailedNoC
         * \brief Detailed model of the NoC integrating BookSim 2 Interconnection Network Simulator
         * 
         * It models a network that delegates all the packet management to BookSim. Each received packet
         * is generated in BookSim and BookSim manage its progress over the NoC, when the packet arrives
         * to destination node, BookSim delivers the packet again to Coyote.
         */
    public:
        /*!
         * \class DetailedNoCParameterSet
         * \brief Parameters for detailed NoC
         */
        class DetailedNoCParameterSet : public NoCParameterSet
        {
        public:
            //! Constructor for DetailedNoCParameterSet
            DetailedNoCParameterSet(sparta::TreeNode* node) :
                NoCParameterSet(node)
            {
            }
            PARAMETER(string, booksim_configuration, "4x4_mesh_iq.cfg", "The configuration file to load by BookSim")
            PARAMETER(vector<uint16_t>, mcpus_indices, {0}, "The indices of MCPUs in the network ordered by MCPU")
            PARAMETER(vector<uint16_t>, network_width, vector<uint16_t>({584,80,80}), "Physical channel width for the networks (bits)")
            PARAMETER(string, stats_files_prefix, "booksimstats", "The prefix of the booksim output statistics file")
        };

        /*!
         * \brief Constructor for DetailedNoC
         * \param node The node that represent the NoC
         * \param params The DetailedNoC parameter set
         */
        DetailedNoC(sparta::TreeNode* node, const DetailedNoCParameterSet* params);

        //! Destructor of DetailedNoC
        ~DetailedNoC();

        /*!
         * \brief Simulate a cycle of BookSim and update the simulation time of BookSim if multiple cycles are required to simulate.
         * 
         * Executes a cycle of BookSim simulator. After that, it tries to retire a packet from the NoC and return if there are packets in NoC.
         * If are called with cycles > 2, it updates the BookSim internal clock
         * 
         * \param cycles The number of cycles to simulate (typically 1)
         * \param current_cycle The current clock managed by simulator_orchestrator
         * \return true If must be executed on the next Coyote cycle
         * \return false If there is no packet in BookSim network and could not be executed in the next Coyote cycle
         * \note This function is not executed under Sparta management, so, the getClock()->currentCycle() is pointing to the latest+1 cycle managed by Sparta
         */
        bool runBookSimCycles(const uint16_t cycles, const uint64_t current_cycle);

    private:

        /*! 
         * \brief Forwards a message from TILE to the actual destination using BookSim
         * \param mess The message to handle
         */
        void handleMessageFromTile_(const shared_ptr<NoCMessage> & mess) override;
        
        /*! 
         * \brief Forwards a message from a MCPU to the correct destination using BookSim
         * \param mess The message to handle
         */
        void handleMessageFromMemoryCPU_(const shared_ptr<NoCMessage> & mess) override;

        string                                  booksim_configuration_;     //! The configuration file to load in BookSim
        vector<Booksim::BooksimWrapper*>        booksim_wrappers_;          //! BookSim library pointer for each NoC network
        uint16_t                                size_;                      //! The network size
        vector<uint16_t>                        mcpu_to_network_;           //! The network ids of mcpus
        vector<uint16_t>                        tile_to_network_;           //! The network ids of tiles
        vector<bool>                            network_is_mcpu_;           //! A network id is a MCPU or a TILE
        vector<uint16_t>                        network_width_;             //! Physical channel width (bits)
        string                                  stats_files_prefix_;        //! The prefix of the output statistics files
        vector<map<int,shared_ptr<NoCMessage>>> pkts_map_;                  //! Map that contains in-flight packets and their messages
        sparta::Counter                         hop_count_data_transfer_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "hop_count_datatransfer",                           // name
            "Total number of packet hops in Data-transfer NoC", // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! Tracks the hop count in data-transfer NoC
        sparta::Counter                         hop_count_address_only_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "hop_count_addressonly",                            // name
            "Total number of packet hops in Address-only NoC",  // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! Tracks the hop count in address-only NoC
        sparta::Counter                         hop_count_control_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "hop_count_control",                                // name
            "Total number of packet hops in Control NoC",       // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! Tracks the hop count in control NoC
        sparta::StatisticDef                    average_hop_count_data_transfer_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "average_hop_count_datatransfer",                   // name
            "Average hop count in Data-transfer NoC",           // description
            getStatisticSet(),                                  // context
            "hop_count_datatransfer/sent_packets_datatransfer"// Expression
        );                                                                  //! The average hop count in data-transfer NoC (counts crossed routers)
        sparta::StatisticDef                    average_hop_count_address_only_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "average_hop_count_addressonly",                    // name
            "Average hop count in Address-only NoC",            // description
            getStatisticSet(),                                  // context
            "hop_count_addressonly/sent_packets_addressonly"// Expression
        );                                                                  //! The average hop count in address-only NoC (counts crossed routers)
        sparta::StatisticDef                    average_hop_count_control_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "average_hop_count_control",                        // name
            "Average hop count in Control NoC",                 // description
            getStatisticSet(),                                  // context
            "hop_count_control/sent_packets_control"            // Expression
        );                                                                  //! The average hop count in control NoC (counts crossed routers)
        sparta::Counter                         count_rx_flits_data_transfer_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "received_flits_datatransfer",                      // name
            "Number of flits received in Data-transfer NoC",    // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The number of packets received in data-transfer NoC
        sparta::Counter                         count_tx_flits_data_transfer_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "sent_flits_datatransfer",                         // name
            "Number of flits sent in Data-transfer NoC",        // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The number of packets sent in data-transfer NoC
        sparta::Counter                         count_rx_flits_address_only_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "received_flits_addressonly",                       // name
            "Number of flits received in Address-only NoC",     // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The number of packets received in address-only NoC
        sparta::Counter                         count_tx_flits_address_only_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "sent_flits_addressonly",                          // name
            "Number of flits sent in Address-only NoC",        // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The number of packets sent in address-only NoC
        sparta::Counter                         count_rx_flits_control_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "received_flits_control",                           // name
            "Number of flits received in Control NoC",          // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The number of packets received in control NoC
        sparta::Counter                         count_tx_flits_control_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "sent_flits_control",                               // name
            "Number of flits sent in Control NoC",              // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The number of packets sent in control NoC
        sparta::StatisticDef                    load_flits_data_transfer_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "load_flits_datatransfer",                          // name
            "Load in Data-transfer NoC (flit/node/cycle)",      // description
            getStatisticSet(),                                  // context
            "received_flits_datatransfer/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                                                  //! Load in data-transfer NoC as flits/nodes/cycles
        sparta::StatisticDef                    load_flits_address_only_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "load_flits_addressonly",                           // name
            "Load in Address-only NoC (flit/node/cycle)",       // description
            getStatisticSet(),                                  // context
            "received_flits_addressonly/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                                                  //! Load in address-only NoC as flits/nodes/cycles
        sparta::StatisticDef                    load_flits_control_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "load_flits_control",                               // name
            "Load in Control NoC (flit/node/cycle)",            // description
            getStatisticSet(),                                  // context
            "received_flits_control/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                                                  //! Load in control NoC as flits/nodes/cycles
        sparta::Counter                         packet_latency_data_transfer_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "packet_latency_datatransfer",                      // name
            "Accumulated packet latency in Data-transfer NoC",  // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The accumulated packet latency in data-transfer NoC
        sparta::Counter                         packet_latency_address_only_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "packet_latency_addressonly",                       // name
            "Accumulated packet latency in Address-only NoC",   // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The accumulated packet latency in address-only NoC
        sparta::Counter                         packet_latency_control_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "packet_latency_control",                           // name
            "Accumulated packet latency in Control NoC",        // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The accumulated packet latency in control NoC
        sparta::StatisticDef                    avg_packet_lat_data_transfer_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "avg_pkt_latency_datatransfer",                     // name
            "Avg. Packet Latency in Data-transfer NoC",         // description
            getStatisticSet(),                                  // context
            "packet_latency_datatransfer/received_packets_datatransfer" // Expression
        );                                                                  //! Average packet latency in data-transfer NoC
        sparta::StatisticDef                    avg_packet_lat_address_only_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "avg_pkt_latency_address_only",                     // name
            "Avg. Packet Latency in Address-only NoC",          // description
            getStatisticSet(),                                  // context
            "packet_latency_addressonly/received_packets_addressonly" // Expression
        );                                                                  //! Average packet latency in address_only NoC
        sparta::StatisticDef                    avg_packet_lat_control_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "avg_pkt_latency_control",                          // name
            "Avg. Packet Latency in Control NoC",               // description
            getStatisticSet(),                                  // context
            "packet_latency_control/received_packets_control"   // Expression
        );                                                                  //! Average packet latency in control NoC
        sparta::Counter                         network_latency_data_transfer_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "network_latency_datatransfer",                     // name
            "Accumulated network latency in Data-transfer NoC", // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The accumulated network latency in data-transfer NoC
        sparta::Counter                         network_latency_address_only_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "network_latency_addressonly",                      // name
            "Accumulated network latency in Address-only NoC",  // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The accumulated network latency in address-only NoC
        sparta::Counter                         network_latency_control_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "network_latency_control",                          // name
            "Accumulated network latency in Control NoC",       // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                                  //! The accumulated network latency in control NoC
        sparta::StatisticDef                    avg_network_lat_data_transfer_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "avg_net_latency_datatransfer",                     // name
            "Avg. Network Latency in Data-transfer NoC",        // description
            getStatisticSet(),                                  // context
            "network_latency_datatransfer/received_packets_datatransfer" // Expression
        );                                                                  //! Average network latency in data-transfer NoC
        sparta::StatisticDef                    avg_network_lat_address_only_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "avg_net_latency_addressonly",                      // name
            "Avg. Network Latency in Address-only NoC",         // description
            getStatisticSet(),                                  // context
            "network_latency_addressonly/received_packets_addressonly" // Expression
        );                                                                  //! Average network latency in address-only NoC
        sparta::StatisticDef                    avg_network_lat_control_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "avg_net_latency_control",                          // name
            "Avg. Network Latency in Control NoC",              // description
            getStatisticSet(),                                  // context
            "network_latency_control/received_packets_control"  // Expression
        );                                                                  //! Average network latency in control NoC
        sparta::Counter                         min_space_in_inj_queue_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "min_space_in_inj_queue",                           // name
            "Minimum space seen in injection queues",           // description
            sparta::Counter::COUNT_LATEST                       // behavior
        );                                                                  //! The minimum space seen in any injection queue

    };

} // spike_model

#endif // __DETAILED_NOC_H__
