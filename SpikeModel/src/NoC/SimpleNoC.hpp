
#ifndef __SIMPLE_NOC_H__
#define __SIMPLE_NOC_H__

#include "NoC.hpp"

using std::vector;
using std::string;
using std::pair;

namespace spike_model
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
            PARAMETER(uint16_t, latency_per_hop, 5, "The latency for each hop (router + output_link)")
            PARAMETER(uint16_t, x_size, 2, "The size of X dimension")
            PARAMETER(uint16_t, y_size, 1, "The size of Y dimension")
            PARAMETER(vector<string>, mcpus_location, {"0.0"}, "The coordinates of MCPUs in the NoC mesh like [X.Y,3.0] ordered by MCPU")
            PARAMETER(std::string, packet_count_file_prefix, "packet_count_", "The prefix of SRC and DST packet count filenames")
            PARAMETER(uint32_t, packet_count_periodicity, 100000, "SRC and DST packet count statistics write periodicity")
        };

        /*!
         * \brief Constructor for SimpleNoC
         * \param node The node that represent the NoC and
         * \param params The SimpleNoC parameter set
         */
        SimpleNoC(sparta::TreeNode* node, const SimpleNoCParameterSet* params);

        ~SimpleNoC();

    private:

        /*! \brief Forwards a message from TILE to the actual destination defining the latency based on the calculated number of hops
         *  \param mess The message to handle
         */
        void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess) override;
        
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
        uint16_t                            x_size_;            //! The size of X dimension
        uint16_t                            y_size_;            //! The size of Y dimension
        vector<pair<uint16_t, uint16_t>>    mcpus_coordinates_; //! The coordinates of MCPUs
        vector<pair<uint16_t, uint16_t>>    tiles_coordinates_; //! The coordinates of TILEs
        sparta::Counter                     hop_count_
        {
            getStatisticSet(),              // parent
            "hop_count",                    // name
            "Total number of packet hops",  // description
            sparta::Counter::COUNT_NORMAL   // behavior
        };                                                      //! Tracks the hop count of all packets
        sparta::StatisticDef                average_hop_count_
        {
            getStatisticSet(),              // parent
            "average_hop_count",            // name
            "Average hop count",            // description
            getStatisticSet(),              // context
            "hop_count/sent_packets"        // Expression
        };                                                      //! The average hop count (counts crossed routers)
        vector<vector<uint64_t>>            dst_count_;         //! Accumulates the number of packets to each destination
        vector<vector<uint64_t>>            src_count_;         //! Accumulates the number of packets from each source
        std::string                         pkt_count_prefix_;  //! The filename prefix for SRC and DST packet count
        uint32_t                            pkt_count_period_;  //! The number of cycles to write packet count statistics
        uint16_t                            pkt_count_stage_;   //! The current stage of writting statistics

    };

} // spike_model

#endif // __SIMPLE_NOC_H__
