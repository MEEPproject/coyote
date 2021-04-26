
#ifndef __DETAILED_NOC_H__
#define __DETAILED_NOC_H__

#include "NoC.hpp"
#include "booksim_wrapper.hpp"

using std::string;
using std::vector;
using std::map;

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
            PARAMETER(uint16_t, network_width, 512, "Physical channel width (bits)")
            PARAMETER(std::string, stats_file, "booksim_stats.txt", "The prefix of the booksim output stats file")
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
        void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess) override;
        
        /*! 
         * \brief Forwards a message from a MCPU to the correct destination using BookSim
         * \param mess The message to handle
         */
        void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess) override;

        string                                  booksim_configuration_;     //! The configuration file to load in BookSim
        Booksim::BooksimWrapper*                booksim_wrapper_;           //! BookSim library pointer
        uint16_t                                size_;                      //! The mesh size
        vector<uint16_t>                        mcpu_to_network_;           //! The network ids of mcpus
        vector<uint16_t>                        tile_to_network_;           //! The network ids of tiles
        vector<bool>                            network_is_mcpu_;           //! A network id is a MCPU or a TILE
        uint16_t                                network_width_;             //! Physical channel width (bits)
        string                                  stats_file_;                //! The prefix of the output statistics file
        map<int,std::shared_ptr<NoCMessage>>    pkts_map_;                  //! Map that contains in-flight packets and their messages
        sparta::Counter                         hop_count_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "hop_count",                    // name
            "Total number of packet hops",  // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                                  //! Tracks the hop count of all packets
        sparta::StatisticDef                    average_hop_count_ = sparta::StatisticDef
        (
            getStatisticSet(),              // parent
            "average_hop_count",            // name
            "Average hop count",            // description
            getStatisticSet(),              // context
            "hop_count/sent_packets"        // Expression
        );                                                                  //! The average hop count (counts crossed routers)
        sparta::Counter                         count_rx_flits_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "received_flits",               // name
            "Number of flits received",     // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                                  //! The number of packets received by the NoC
        sparta::Counter                         count_tx_flits_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "sent_flits",                   // name
            "Number of flits sent",         // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                                  //! The number of packets sent by the NoC
        sparta::StatisticDef                    load_flits_ = sparta::StatisticDef
        (
            getStatisticSet(),              // parent
            "load_flits",                   // name
            "NoC load (flit/node/cycle)",   // description
            getStatisticSet(),              // context
            "received_flits/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                                                  //! NoC load as flits/nodes/cycles
        sparta::Counter                         packet_latency_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "packet_latency",               // name
            "Aggregated packet latency",    // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                                  //! The accumulated packet latency
        sparta::StatisticDef                    avg_packet_lat = sparta::StatisticDef
        (
            getStatisticSet(),                  // parent
            "avg_pkt_latency",                  // name
            "Avg. Packet Latency",              // description
            getStatisticSet(),                  // context
            "packet_latency/received_packets"   // Expression
        );                                                                  //! Average packet latency
        sparta::Counter                         network_latency_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "network_latency",              // name
            "Aggregated network latency",   // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                                  //! The accumulated network latency
        sparta::StatisticDef                    avg_network_lat = sparta::StatisticDef
        (
            getStatisticSet(),                  // parent
            "avg_net_latency",                  // name
            "Avg. Network Latency",             // description
            getStatisticSet(),                  // context
            "network_latency/received_packets"  // Expression
        );                                                                  //! Average network latency

    };

} // spike_model

#endif // __DETAILED_NOC_H__
