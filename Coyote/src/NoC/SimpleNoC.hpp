
#ifndef __SIMPLE_NOC_H__
#define __SIMPLE_NOC_H__

#include "NoC.hpp"

using std::vector;
using std::string;
using std::pair;

namespace coyote
{
    class SimpleNoC : public NoC
    {
        /*!
         * \class SimpleNoC
         * \brief Simple model of the NoC with a mesh topology which calculates the average hop count and generates traffic maps
         * 
         * It models a MESH with a variable location of the MCPUs and DOR to calculate the hop count
         */
    public:
        /*!
         * \class SimpleNoCParameterSet
         * \brief Parameters for simple NoC
         */
        class SimpleNoCParameterSet : public NoCParameterSet
        {
        public:
            //! Constructor for SimpleNoCParameterSet
            SimpleNoCParameterSet(sparta::TreeNode* node) :
                NoCParameterSet(node)
            {
            }
            PARAMETER(uint16_t, latency_per_hop, 4, "The latency for each hop (router + output_link)")
            PARAMETER(std::string, packet_count_file_prefix, "packet_count_", "The prefix of SRC and DST packet count filenames")
            PARAMETER(uint32_t, packet_count_periodicity, 100000, "SRC and DST packet count statistics write periodicity")
            PARAMETER(bool, flush_packet_count_each_period, false, "Flush the packet count statistics or accumulate them")
        };

        /*!
         * \brief Constructor for SimpleNoC
         * \param node The node that represent the NoC and
         * \param params The SimpleNoC parameter set
         */
        SimpleNoC(sparta::TreeNode* node, const SimpleNoCParameterSet* params);

        ~SimpleNoC();

        /*!
         * \brief Check the injection queue space in NoC for a packet
         * \param injectedByTile Indicates that the source of the messages is a VAS tile
         * \param mess The packet
         * \return If there is space for the packet on its corresponding injection queue
         */
        virtual bool checkSpaceForPacket(const bool injectedByTile, const std::shared_ptr<NoCMessage> & mess) override;

        /*! \brief Forwards a message from TILE to the actual destination defining the latency based on the calculated number of hops
         *  \param mess The message to handle
         */
        void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess) override;

    private:
        
        /*! \brief Forwards a message from a MCPU to the actual destination defining the latency based on the calculated number of hops
         *  \param mess The message to handle
         */
        void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess) override;

        /*!
         * \brief Write the SRC and DST packet count matrix to a file each stats_periodicity_ cycles
         * 
         */
        void writePacketCountMatrix_();

        uint16_t                            latency_per_hop_;   //! The latency for each hop
        vector<pair<uint16_t, uint16_t>>    mcpus_coordinates_; //! The coordinates of MCPUs
        vector<pair<uint16_t, uint16_t>>    tiles_coordinates_; //! The coordinates of TILEs
        std::vector<sparta::Counter>        hop_count_;         //! Tracks the hop count for each NoC network
        std::vector<sparta::StatisticDef>   average_hop_count_; //! The average hop count for each NoC (counts crossed routers)
        //[y][x][network]
        vector<vector<vector<uint64_t>>>    dst_count_;         //! Accumulates the number of packets to each destination by NoC network
        vector<vector<vector<uint64_t>>>    src_count_;         //! Accumulates the number of packets from each source by NoC network
        //DST[y][x]SRC[y][x][network]
        vector<vector<vector<vector<vector<uint64_t>>>>> dst_src_count_; //! Accumulates the number of packets from each src to each dst by NoC network
        std::string                         pkt_count_prefix_;  //! The filename prefix for SRC and DST packet count
        uint32_t                            pkt_count_period_;  //! The number of cycles to write packet count statistics
        uint16_t                            pkt_count_stage_;   //! The current stage of writting statistics
        bool                                pkt_count_flush_;    //! Flush or accumulate packet counts

    };

} // coyote

#endif // __SIMPLE_NOC_H__
