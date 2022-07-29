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


#include "FunctionalNoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"

using std::vector;
namespace coyote
{
    FunctionalNoC::FunctionalNoC(sparta::TreeNode *node, const FunctionalNoCParameterSet *params) :
        NoC(node, params),
        packet_latency_(params->packet_latency)
    {
        sparta_assert(noc_model_ == "functional");
    }

    bool FunctionalNoC::checkSpaceForPacket(const bool injectedByTile, const std::shared_ptr<NoCMessage> & mess){return true;}

    void FunctionalNoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the statistics
        NoC::handleMessageFromTile_(mess);
        switch(mess->getType())
        {
            // VAS -> VAS messages
            case NoCMessageType::REMOTE_L2_REQUEST:
            case NoCMessageType::REMOTE_L2_ACK:
                vas_queue_.at(mess->getNoCNetwork()).at(mess->getDstPort()).push_back(std::make_pair(mess, getClock()->currentCycle() + packet_latency_));
                break;

            // VAS -> MCPU messages
            case NoCMessageType::MEMORY_REQUEST_LOAD:
            case NoCMessageType::MEMORY_REQUEST_STORE:
            case NoCMessageType::MEMORY_REQUEST_WB:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_ACK:
            case NoCMessageType::SCRATCHPAD_DATA_REPLY:
                mem_queue_.at(mess->getNoCNetwork()).at(mess->getDstPort()).push_back(std::make_pair(mess, getClock()->currentCycle() + packet_latency_));
                break;

            default:
                sparta_assert(false);
        }
    }

    void FunctionalNoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the statistics
        NoC::handleMessageFromMemoryCPU_(mess);
        switch(mess->getType())
        {
            // MemoryTile -> VAS messages
            case NoCMessageType::MEMORY_ACK:
            case NoCMessageType::MCPU_REQUEST:
            case NoCMessageType::SCRATCHPAD_COMMAND:
                vas_queue_.at(mess->getNoCNetwork()).at(mess->getDstPort()).push_back(std::make_pair(mess, getClock()->currentCycle() + packet_latency_));
                break;
            // MemoryTile -> Memory Tile Communication
            case NoCMessageType::MEM_TILE_REQUEST:
            case NoCMessageType::MEM_TILE_REPLY:
                mem_queue_.at(mess->getNoCNetwork()).at(mess->getDstPort()).push_back(std::make_pair(mess, getClock()->currentCycle() + packet_latency_));
                break;

            default:
                sparta_assert(false);
        }
    }

} // coyote
