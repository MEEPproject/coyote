
#ifndef __NoC_H__
#define __NoC_H__

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"
#include "sparta/resources/Pipeline.hpp"
#include "sparta/resources/Buffer.hpp"
#include "sparta/pairs/SpartaKeyPairs.hpp"
#include "sparta/simulation/State.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"

#include <memory>

#include "NoCMessage.hpp"
#include "../LogCapable.hpp"

namespace spike_model
{
    //class Core; //Forward declaration

    class NoC : public sparta::Unit, public LogCapable
    {
        /*!
         * \class spike_model::NoC
         * \brief NoC models the network on chip interconnecting tiles and memory cpus
         * 
         * ABC to define the interface between PEs and NoC
         */
    public:
        /*!
         * \class NoCParameterSet
         * \brief Parameters for NoC
         */
        class NoCParameterSet : public sparta::ParameterSet
        {

        public:
        
            //! Constructor for NoCParameterSet
            NoCParameterSet(sparta::TreeNode* n):
                sparta::ParameterSet(n)
            {
            }
            PARAMETER(uint16_t, num_tiles, 1, "The number of tiles")
            PARAMETER(uint16_t, num_memory_cpus, 1, "The number of memory CPUs")
            PARAMETER(std::string, noc_model, "functional", "The noc model to use (functional, simple, detailed)")
        };

        //! name of this resource.
        static const char name[];

        /*! \brief Forward a message sent from a tile to the correct destination
        *   \param mes The meesage to handle
        */
        virtual void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mes);

        /*! \brief Forward a message sent from a memory CPU to the correct destination
        * \param mes The message
        */
        virtual void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mes);
 
    protected:

        /*!
         * \brief Constructor for NoC
         * \param node The node that represent the NoC and
         * \param p The NoC parameter set
         */
        NoC(sparta::TreeNode* node, const NoCParameterSet* params);

        ~NoC() 
        {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << std::endl;
        }

        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>>> in_ports_tiles_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>>> out_ports_tiles_;
        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>>> in_ports_memory_cpus_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>>> out_ports_memory_cpus_;
        std::string noc_model_;         //! The model of NoC to simulate
        uint16_t    num_tiles_;         //! The number of tiles connected
        uint16_t    num_memory_cpus_;   //! The number of memory cpus connected
        /* Statistics */
        sparta::Counter count_rx_packets_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "received_packets",             // name
            "Number of packets received",   // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of packets received by the NoC
        sparta::Counter count_tx_packets_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "sent_packets",                 // name
            "Number of packets sent",       // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of packets sent by the NoC

    private:

        sparta::StatisticDef load_pkt_ = sparta::StatisticDef
        (
            getStatisticSet(),              // parent
            "load_pkt",                     // name
            "NoC load (pkt/node/cycle)",    // description
            getStatisticSet(),              // context
            "received_packets/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                  //! Basic NoC load as packets/cycles
        sparta::Counter count_remote_l2_requests_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "remote_l2_requests",           // name
            "Number of Remote_L2_Request",  // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of Remote_L2_Requests forwarded by NoC
        sparta::Counter count_remote_l2_acks_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "remote_l2_acks",               // name
            "Number of Remote_L2_Acks",     // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of Remote_L2_Acks forwarded by NoC
        sparta::Counter count_memory_requests_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "memory_requests",              // name
            "Number of Memory_Request",     // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of Memory_Requests forwarded by NoC
        sparta::Counter count_memory_acks_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "memory_acks",                  // name
            "Number of Memory_Acks",        // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of Memory_Acks forwarded by NoC
        sparta::Counter count_mcpu_requests_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "mcpu_requests",                // name
            "Number of MCPU_Requests",      // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of MCPU_Requests forwarded by NoC
        sparta::Counter count_scratchpad_acks_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "scratchpad_acks",              // name
            "Number of Scratchpad_Acks",    // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                  //! The number of Scratchpad_Acks forwarded by NoC

    };

} // spike_model

#endif // __NoC_H__
