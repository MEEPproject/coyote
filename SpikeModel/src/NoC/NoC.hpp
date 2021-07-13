
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
#include "NoCMessageType.hpp"
#include "../LogCapable.hpp"

namespace spike_model
{

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
            PARAMETER(uint8_t, header_size, 8, "The header size of the messages (in bits)")
            PARAMETER(std::vector<std::string>, message_to_network_and_class, std::vector<std::string>(
                {"REMOTE_L2_REQUEST:ADDRESS_ONLY.1",
                 "MEMORY_REQUEST_LOAD:ADDRESS_ONLY.1",
                 "MEMORY_REQUEST_STORE:DATA_TRANSFER.0",
                 "MEMORY_REQUEST_WB:DATA_TRANSFER.0",
                 "REMOTE_L2_ACK:DATA_TRANSFER.1",
                 "MEMORY_ACK:DATA_TRANSFER.1",
                 "MCPU_REQUEST:ADDRESS_ONLY.0",
                 "SCRATCHPAD_ACK:CONTROL.0",
                 "SCRATCHPAD_DATA_REPLY:DATA_TRANSFER.2",
                 "SCRATCHPAD_COMMAND:DATA_TRANSFER.3"}), "Mapping of messages to networks and classes")
        };

        /*!
         * \brief The NoC networks
         * \note The numbers defined by this enum are used in the transaction_type field of message's header and in the statistics
         */
        enum class Networks
        {
            DATA_TRANSFER_NOC   = 0,
            ADDRESS_ONLY_NOC    = 1,
            CONTROL_NOC         = 2,
            count               = 3 // Number of modelled NoCs
        };

        //! name of this resource.
        static const char name[];

        static Networks getNetworkForMessage(const NoCMessageType mess);
        static uint8_t getClassForMessage(const NoCMessageType mess);

    protected:

        /*! \brief Forward a message sent from a tile to the correct destination
        *   \param mes The meesage to handle
        */
        virtual void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mes);

        /*! \brief Forward a message sent from a memory CPU to the correct destination
        * \param mes The message
        */
        virtual void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mes);

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
        std::string                                     noc_model_;                     //! The model of NoC to simulate
        uint16_t                                        num_tiles_;                     //! The number of tiles connected
        uint16_t                                        num_memory_cpus_;               //! The number of memory cpus connected
        uint8_t                                         max_class_used_;                //! The maximum class value used in messages
        /* Statistics */
        sparta::Counter count_rx_packets_data_transfer_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "received_packets_datatransfer",                    // name
            "Number of packets received in Data-transfer NoC",  // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                      //! The number of packets received in data-transfer NoC
        sparta::Counter count_tx_packets_data_transfer_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "sent_packets_datatransfer",                        // name
            "Number of packets sent in Data-transfer NoC",      // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                      //! The number of packets sent in data-transfer NoC
        sparta::Counter count_rx_packets_address_only_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "received_packets_addressonly",                     // name
            "Number of packets received in Address-only NoC",   // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                      //! The number of packets received in address-only NoC
        sparta::Counter count_tx_packets_address_only_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "sent_packets_addressonly",                         // name
            "Number of packets sent in Address-only NoC",       // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                      //! The number of packets sent in address-only NoC
        sparta::Counter count_rx_packets_control_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "received_packets_control",                         // name
            "Number of packets received in Control NoC",        // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                      //! The number of packets received in control NoC
        sparta::Counter count_tx_packets_control_ = sparta::Counter
        (
            getStatisticSet(),                                  // parent
            "sent_packets_control",                             // name
            "Number of packets sent in Control NoC",            // description
            sparta::Counter::COUNT_NORMAL                       // behavior
        );                                                      //! The number of packets sent in control NoC

    private:

        /*!
         * \brief Get the Network From String object
         * 
         * \param net String representation of the network
         * \return Networks 
         */
        Networks getNetworkFromString_(const std::string& net)
        {
            if (net == "DATA_TRANSFER" || net == "DATA_TRANSFER_NOC" || net == "data_transfer") return Networks::DATA_TRANSFER_NOC;
            else if (net == "ADDRESS_ONLY" || net == "ADDRESS_ONLY_NOC" || net == "address_only") return Networks::ADDRESS_ONLY_NOC;
            else if (net == "CONTROL" || net == "CONTROL_NOC" || net == "control") return Networks::CONTROL_NOC;
            else sparta_assert(false, "Network " + net + " not valid, see NoC::Networks.");
        }

        /*!
         * \brief Get the NoC Message Type From String object
         * 
         * \param mess representation of message
         * \return NoCMessageType 
         */
        NoCMessageType getMessageTypeFromString_(const std::string& mess)
        {
            if (mess == "REMOTE_L2_REQUEST") return NoCMessageType::REMOTE_L2_REQUEST;
            else if (mess == "MEMORY_REQUEST_LOAD") return NoCMessageType::MEMORY_REQUEST_LOAD;
            else if (mess == "MEMORY_REQUEST_STORE") return NoCMessageType::MEMORY_REQUEST_STORE;
            else if (mess == "MEMORY_REQUEST_WB") return NoCMessageType::MEMORY_REQUEST_WB;
            else if (mess == "REMOTE_L2_ACK") return NoCMessageType::REMOTE_L2_ACK;
            else if (mess == "MEMORY_ACK") return NoCMessageType::MEMORY_ACK;
            else if (mess == "MCPU_REQUEST") return NoCMessageType::MCPU_REQUEST;
            else if (mess == "SCRATCHPAD_ACK") return NoCMessageType::SCRATCHPAD_ACK;
            else if (mess == "SCRATCHPAD_DATA_REPLY") return NoCMessageType::SCRATCHPAD_DATA_REPLY;
            else if (mess == "SCRATCHPAD_COMMAND") return NoCMessageType::SCRATCHPAD_COMMAND;
            else sparta_assert(false, "Message " + mess + " not defined. See NoCMessageType.");
        }

        static std::map<NoCMessageType, std::pair<Networks,uint8_t>> message_to_network_and_class_;  //! The mapping of messages to networks and classes

        sparta::StatisticDef load_pkt_data_transfer_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "load_pkt_datatransfer",                            // name
            "Load in Data-transfer NoC (pkt/node/cycle)",       // description
            getStatisticSet(),                                  // context
            "received_packets_datatransfer/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                                      //! Basic Data-transfer NoC load as packets/nodes/cycles
        sparta::StatisticDef load_pkt_address_only_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "load_pkt_addressonly",                             // name
            "Load in Address-only NoC (pkt/node/cycle)",        // description
            getStatisticSet(),                                  // context
            "received_packets_addressonly/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                                      //! Basic Address-only NoC load as packets/nodes/cycles
        sparta::StatisticDef load_pkt_control_ = sparta::StatisticDef
        (
            getStatisticSet(),                                  // parent
            "load_pkt_control",                                 // name
            "Load in Control NoC (pkt/node/cycle)",             // description
            getStatisticSet(),                                  // context
            "received_packets_control/("+std::to_string(num_tiles_+num_memory_cpus_)+"*cycles)" // Expression
        );                                                      //! Basic Control NoC load as packets/nodes/cycles
        sparta::Counter count_remote_l2_requests_ = sparta::Counter
        (
            getStatisticSet(),                  // parent
            "remote_l2_requests",               // name
            "Number of Remote_L2_Request",      // description
            sparta::Counter::COUNT_NORMAL       // behavior
        );                                                      //! The number of Remote_L2_Requests forwarded by NoC
        sparta::Counter count_memory_requests_wb_ = sparta::Counter
        (
            getStatisticSet(),                  // parent
            "memory_requests_wb",                  // name
            "Number of Memory_Request_WriteBack",         // description
            sparta::Counter::COUNT_NORMAL       // behavior
        );                                                      //! The number of Memory_Requests_WriteBack forwarded by NoC
        sparta::Counter count_memory_requests_load_ = sparta::Counter
        (
            getStatisticSet(),                  // parent
            "memory_requests_load",             // name
            "Number of Memory_Request_Load",    // description
            sparta::Counter::COUNT_NORMAL       // behavior
        );                                                      //! The number of Memory_Requests_Load forwarded by NoC
        sparta::Counter count_memory_requests_store_ = sparta::Counter
        (
            getStatisticSet(),                  // parent
            "memory_requests_store",            // name
            "Number of Memory_Request_Store",   // description
            sparta::Counter::COUNT_NORMAL       // behavior
        );                                                      //! The number of Memory_Requests_Store forwarded by NoC
        sparta::Counter count_remote_l2_acks_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "remote_l2_acks",               // name
            "Number of Remote_L2_Acks",     // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                      //! The number of Remote_L2_Acks forwarded by NoC
        sparta::Counter count_memory_acks_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "memory_acks",                  // name
            "Number of Memory_Acks",        // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                      //! The number of Memory_Acks forwarded by NoC
        sparta::Counter count_mcpu_requests_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "mcpu_requests",                // name
            "Number of MCPU_Requests",      // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                      //! The number of MCPU_Requests forwarded by NoC
        sparta::Counter count_scratchpad_acks_ = sparta::Counter
        (
            getStatisticSet(),              // parent
            "scratchpad_acks",              // name
            "Number of Scratchpad_Acks",    // description
            sparta::Counter::COUNT_NORMAL   // behavior
        );                                                      //! The number of Scratchpad_Acks forwarded by NoC
        sparta::Counter count_scratchpad_data_replies_ = sparta::Counter
        (
            getStatisticSet(),                  // parent
            "scratchpad_data_replies",          // name
            "Number of Scratchpad_Data_Reply",  // description
            sparta::Counter::COUNT_NORMAL       // behavior
        );                                                      //! The number of Scratchpad_Data_Reply forwarded by NoC
        sparta::Counter count_scratchpad_commands_ = sparta::Counter
        (
            getStatisticSet(),                  // parent
            "scratchpad_commands",              // name
            "Number of Scratchpad_Command",     // description
            sparta::Counter::COUNT_NORMAL       // behavior
        );                                                      //! The number of Scratchpad_Commands forwarded by NoC
        
        /*! \brief Trace the source a destination for a message
        * \param mes The message
        */
        void traceSrcDst_(const std::shared_ptr<NoCMessage> & mess);

    };
    
} // spike_model

#endif // __NoC_H__
