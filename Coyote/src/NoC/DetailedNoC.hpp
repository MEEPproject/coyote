
#ifndef __DETAILED_NOC_H__
#define __DETAILED_NOC_H__

#include "NoC.hpp"
#include "booksim_wrapper.hpp"

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;

namespace coyote
{

    class MemoryCPUWrapper; // forward declaration

    class DetailedNoC : public NoC
    {
        /*!
         * \class coyote::DetailedNoC
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
         * \brief Check the injection queue space in NoC for a packet
         * \param injectedByTile Indicates that the source of the messages is a VAS tile
         * \param mess The packet
         * \return If there is space for the packet on its corresponding injection queue
         */
        virtual bool checkSpaceForPacket(const bool injectedByTile, const std::shared_ptr<NoCMessage> & mess) override;

        /*!
         * \brief Simulate a cycle of BookSim and update the simulation time of BookSim if multiple cycles are required to simulate.
         * 
         * Executes a cycle of BookSim simulator. After that, it tries to retire a packet from the NoC and return if there are packets in NoC.
         * If are called with cycles > 2, it updates the BookSim internal clock
         * 
         * \param cycles The number of cycles to simulate (typically 1)
         * \note This function is not executed under Sparta management, so, the getClock()->currentCycle() is pointing to the latest+1 cycle managed by Sparta
         */
        virtual void runBookSimCycles(const uint16_t cycles) override;
        /*!
         * \brief Extract, at maximum, one packet for each destination in each network and send them through ports
         * \param current_cycle The current clock managed by simulator_orchestrator
         * \return true If must be executed on the next Coyote cycle
         * \return false If there is no packet in network and could not be executed in the next Coyote cycle
         * \note This function is not executed under Sparta management, so, the getClock()->currentCycle() is pointing to the latest+1 cycle managed by Sparta
         */
        virtual bool deliverOnePacketToDestination(const uint64_t current_cycle) override;

         /*! 
         * \brief Forwards a message from TILE to the actual destination using BookSim
         * \param mess The message to handle
         */
        void handleMessageFromTile_(const shared_ptr<NoCMessage> & mess) override;

    private:

        /*! 
         * \brief Forwards a message from a MCPU to the correct destination using BookSim
         * \param mess The message to handle
         */
        void handleMessageFromMemoryCPU_(const shared_ptr<NoCMessage> & mess) override;

        string                                  booksim_configuration_;     //! The configuration file to load in BookSim
        uint16_t                                ejection_queue_size_;       //! The configured ejection queue size
        vector<Booksim::BooksimWrapper*>        booksim_wrappers_;          //! BookSim library pointer for each NoC network
        vector<uint16_t>                        mcpu_to_network_;           //! The network ids of mcpus
        vector<uint16_t>                        tile_to_network_;           //! The network ids of tiles
        vector<bool>                            network_is_mcpu_;           //! A network id is a MCPU or a TILE
        vector<uint16_t>                        network_width_;             //! Physical channel width (bits)
        string                                  stats_files_prefix_;        //! The prefix of the output statistics files
        vector<map<long,shared_ptr<NoCMessage>>> pkts_map_;                 //! Map that contains in-flight packets and their messages
        std::vector<sparta::Counter>            hop_count_by_noc;           //! Tracks the hop count for each NoC network
        std::vector<sparta::Counter>            count_rx_flits_by_noc;      //! The number of flits received by each NoC
        std::vector<sparta::Counter>            count_tx_flits_by_noc;      //! The number of flits sent in each NoC
        std::vector<sparta::Counter>            packet_latency_by_noc;      //! The accumulated packet latency in each NoC
        std::vector<sparta::Counter>            packet_latency_by_type;     //! The accumulated packet latency for each packet type
        std::vector<sparta::Counter>            network_latency_by_noc;     //! The accumulated network latency in each NoC
        std::vector<sparta::Counter>            network_latency_by_type;     //! The accumulated network latency for each packet type
        std::vector<sparta::Counter>            min_space_in_inj_queue_;    //! The minimum space seen in any injection queue
        std::vector<sparta::StatisticDef>       average_hop_count_by_noc;   //! The average hop count for each NoC (counts crossed routers)
        std::vector<sparta::StatisticDef>       load_flits_by_noc;          //! Load in each NoC as flits/nodes/cycles
        std::vector<sparta::StatisticDef>       avg_packet_lat_by_noc;      //! Average packet latency in each NoC
        std::vector<sparta::StatisticDef>       avg_packet_lat_by_type;     //! Average packet latency for each packet type
        std::vector<sparta::StatisticDef>       avg_network_lat_by_noc;     //! Average network latency in each NoC
        std::vector<sparta::StatisticDef>       avg_network_lat_by_type;    //! Average network latency for each packet type
    };

} // coyote

#endif // __DETAILED_NOC_H__
