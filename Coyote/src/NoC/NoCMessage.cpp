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

#include "NoCMessage.hpp"
#include "NoC.hpp"

#define BYTES_TO_BITS 8

namespace coyote
{
    uint8_t NoCMessage::header_size[static_cast<int>(NoCMessageType::count)]; // Define header_size for each packet

    NoCMessage::NoCMessage(std::shared_ptr<CoreEvent> r, NoCMessageType t, uint16_t payload_size, uint16_t src_port, uint16_t dst_port) :
        request_(r),
        message_type_(t),
        src_port_(src_port),
        dst_port_(dst_port)
    {
        size_= header_size[static_cast<int>(message_type_)] + payload_size + payload_size*BYTES_TO_BITS; // Header (including CRC) + payload + payload ECC
        // Set the noc_network and class
        noc_network_ = static_cast<uint8_t>(NoC::getNetworkForMessage(message_type_));
        class_ = NoC::getClassForMessage(message_type_);
    }

    void NoCMessage::setDstPort(uint16_t dst_port)
    {
        dst_port_ = dst_port;
    }

} // coyote
