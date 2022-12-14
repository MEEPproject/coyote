// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 


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
#include <tuple>
#include <vector>

#include "NoCMessage.hpp"
#include "NoCMessageType.hpp"
#include "../LogCapable.hpp"

using std::vector;
using std::shared_ptr;
using std::pair;

namespace coyote
{

    class MemoryCPUWrapper; // forward declaration

    class NoC : public sparta::Unit, public LogCapable
    {
        /*!
         * \class coyote::NoC
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
            PARAMETER(uint16_t, x_size, 2, "The size of X dimension")
            PARAMETER(uint16_t, y_size, 1, "The size of Y dimension")
            PARAMETER(std::vector<uint16_t>, mcpus_indices, {0}, "The indices of MCPUs in the network ordered by MCPU")
            PARAMETER(std::string, noc_model, "functional", "The noc model to use (functional, simple, detailed)")
            PARAMETER(std::vector<std::string>, noc_networks, std::vector<std::string>(
                {"DATA_TRANSFER",
                 "ADDRESS_ONLY",
                 "CONTROL"}), "The name and implicitly the amount of the networks to define")
            PARAMETER(std::vector<std::string>, message_header_size, std::vector<std::string>(
                {"REMOTE_L2_REQUEST:8",
                 "MEMORY_REQUEST_LOAD:8",
                 "MEMORY_REQUEST_STORE:8",
                 "MEMORY_REQUEST_WB:8",
                 "REMOTE_L2_ACK:8",
                 "MEMORY_ACK:8",
                 "MCPU_REQUEST:8",
                 "SCRATCHPAD_ACK:8",
                 "SCRATCHPAD_DATA_REPLY:8",
                 "SCRATCHPAD_COMMAND:8"}), "The header size of each message including CRC (in bits)")
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
                 "SCRATCHPAD_COMMAND:DATA_TRANSFER.3",
                 "MEM_TILE_REQUEST:DATA_TRANSFER.2",
                 "MEM_TILE_REPLY:DATA_TRANSFER.2"}), "Mapping of messages to networks and classes")
        };

        //! name of this resource.
        static const char name[];

        static uint8_t getNetworkForMessage(const NoCMessageType mess);
        static uint8_t getClassForMessage(const NoCMessageType mess);
        const std::string getNetworkName(const uint8_t noc);
        /*!
         * \brief Check the injection queue space in NoC for a packet
         * \param injectedByTile Indicates that the source of the messages is a VAS tile
         * \param mess The packet
         * \return If there is space for the packet on its corresponding injection queue
         */
        virtual bool checkSpaceForPacket(const bool injectedByTile, const std::shared_ptr<NoCMessage> & mess) = 0;
        /*! \brief Forward a message sent from a tile to the correct destination
        *   \param mes The message to handle
        */
        virtual void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mes);
        /*!
         * \brief Simulate a cycle of BookSim and update the simulation time of BookSim if multiple cycles are required to simulate.
         * 
         * Executes a cycle of BookSim simulator. After that, it tries to retire a packet from the NoC and return if there are packets in NoC.
         * If are called with cycles > 2, it updates the BookSim internal clock
         * 
         * \param cycles The number of cycles to simulate (typically 1)
         * \note This function is not executed under Sparta management, so, the getClock()->currentCycle() is pointing to the latest+1 cycle managed by Sparta
         * \note This function is override in DetailedModel to run BookSim
         */
        virtual inline void runBookSimCycles(const uint16_t cycles) {};
        /*!
         * \brief Extract, at maximum, one packet for each destination in each network and send them through ports
         * \param current_cycle The current clock managed by simulator_orchestrator
         * \return true If must be executed on the next Coyote cycle
         * \return false If there is no packet in network and could not be executed in the next Coyote cycle
         * \note This function is not executed under Sparta management, so, the getClock()->currentCycle() is pointing to the latest+1 cycle managed by Sparta
         * \note THis function is override is Detailed model because this model does not use the {vas,mem}_queue because the interface has its own ejection queue
         */
        virtual bool deliverOnePacketToDestination(const uint64_t current_cycle);

        void setMemoryTiles(std::shared_ptr<std::vector<MemoryCPUWrapper *>> &newMemoryTiles);

    protected:

        /*! \brief Forward a message sent from a memory CPU to the correct destination
        * \param mes The message
        */
        virtual void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mes);

        /*!
         * \brief Constructor for NoC
         * \param node The node that represent the NoC
         * \param p The NoC parameter set
         */
        NoC(sparta::TreeNode* node, const NoCParameterSet* params);

        ~NoC() 
        {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << std::endl;
        }

        /*!
         * \brief Get the Network From String object
         * 
         * \param net String representation of the network
         * \return int 
         */
        uint8_t getNetworkFromString(const std::string& net);

        vector<std::unique_ptr<sparta::DataInPort<shared_ptr<NoCMessage>>>> in_ports_tiles_;
        vector<std::unique_ptr<sparta::DataOutPort<shared_ptr<NoCMessage>>>> out_ports_tiles_;
        vector<std::unique_ptr<sparta::DataInPort<shared_ptr<NoCMessage>>>> in_ports_memory_cpus_;
        vector<std::unique_ptr<sparta::DataOutPort<shared_ptr<NoCMessage>>>> out_ports_memory_cpus_;
        vector<vector<vector<pair<shared_ptr<NoCMessage>, uint64_t>>>> vas_queue_;      //! Packets queue indexed by NoC and VAS destination
        vector<vector<vector<pair<shared_ptr<NoCMessage>, uint64_t>>>> mem_queue_;      //! Packets queue indexed by NoC and MEM destination
        uint16_t                                        num_tiles_;                     //! The number of tiles connected
        uint16_t                                        num_memory_cpus_;               //! The number of memory cpus connected
        uint16_t                                        x_size_;                        //! The size of X dimension
        uint16_t                                        y_size_;                        //! The size of Y dimension
        uint16_t                                        size_;                          //! The network size
        vector<uint16_t>                                mcpus_indices_;                 //! The indices of MCPUs in the network ordered by MCPU
        std::string                                     noc_model_;                     //! The model of NoC to simulate
        uint8_t                                         max_class_used_;                //! The maximum class value used in messages
        vector<std::string>                             noc_networks_;                  //! The name of the defined NoC networks
        /* Statistics */
        vector<sparta::Counter> count_rx_packets_;                                      //! The number of packets received in each NoC
        vector<sparta::Counter> count_tx_packets_;                                      //! The number of packets sent in each NoC
        std::shared_ptr<std::vector<MemoryCPUWrapper *>> memoryTiles;

    private:

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
            else if (mess == "MEM_TILE_REQUEST") return NoCMessageType::MEM_TILE_REQUEST;
            else if (mess == "MEM_TILE_REPLY") return NoCMessageType::MEM_TILE_REPLY;
            else sparta_assert(false, "Message " + mess + " not defined. See NoCMessageType.");
        }

        static std::map<NoCMessageType, std::pair<uint8_t,uint8_t>> message_to_network_and_class_;  //! The mapping of messages to networks and classes

        std::vector<sparta::StatisticDef> load_pkt_;                //! Basic NoC load as packets/nodes/cycles for each network
        std::vector<sparta::Counter>      count_pkts_by_pkt_type;   //! The number of packets received by type
        
        /*! \brief Trace the source a destination for a message
        * \param mes The message
        */
        void traceSrcDst_(const std::shared_ptr<NoCMessage> & mess);

    };
    
} // coyote

#endif // __NoC_H__
