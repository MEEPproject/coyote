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


#ifndef __NOC_MESSAGE_HH__
#define __NOC_MESSAGE_HH__

#include <memory>
#include "NoCMessageType.hpp"
#include "../CoreEvent.hpp"
#include <iostream>

namespace coyote
{
    class NoCMessage
    {
        /*!
         * \class coyote::NoCMessage
         * \brief A representation of a message that wille travel through the NoC.
         *
         * NoCMessages are associated to a particular CacheRequest and hold extra information regarding the directionality and size of the message
         */
        public:
            NoCMessage() = delete;
            NoCMessage(NoCMessage const&) = delete;
            NoCMessage& operator=(NoCMessage const&) = delete;
            
            /*!
             * \brief Constructor for NoCMessage. NoCMessages should only be created for requests with full memory destination info. For this reason they are created in class EventManager. 
             * Think twice before creating directly elsewhere.
             * \param r The associated CacheRequest
             * \param t The type of the message
             * \param payload_size The size in bytes of the data to be sent
             * \param src_port The source port that depends on the type of message
             * \param dst_port The destination port that depends on the type of message
             * \note NoCMessages must contain fully initialized Requests, including all of the associated destination data. Handle with care.
             */
            NoCMessage(std::shared_ptr<CoreEvent> r, NoCMessageType t, uint16_t payload_size, uint16_t src_port, uint16_t dst_port);

            /*!
             * \brief Get the type of the message
             * \return The type
             */
            NoCMessageType getType(){return message_type_;}
            
            /*!
             * \brief Get the CacheRequest associated to the message
             * \return The CacheRequest
             */
            std::shared_ptr<CoreEvent> getRequest(){return request_;}

            
            /*!
             * \brief Get the size of the message (including its header and ECC size)
             * \return The size in bits
             */
            uint16_t getSize(){return size_;};
            
            /*!
             * \brief Get the destination port that depends on the type of message
             * 
             * @return uint16_t Destination port
             */
            uint16_t getDstPort(){return dst_port_;}

            /*!
             * @brief Set the destination port
             * 
             * @param dstPort Destination port
             */
            void setDstPort(uint16_t dst_port);

            /*!
             * @brief Get the source port that depends on the type of message
             * 
             * @return uint16_t Source port
             */
            uint16_t getSrcPort(){return src_port_;}

            /*!
             * \brief Get the noc network to use
             * \see NoC::noc_networks_
             * \return NoC Network
             */
            uint8_t getNoCNetwork(){return noc_network_;}

            /*!
             * \brief Get the class of the message
             * 
             * @return Class (priority, VC or both)
             */
            uint8_t getClass(){return class_;}

            static uint8_t header_size[static_cast<int>(NoCMessageType::count)]; //! The size of the header for each message in bits

        private:

            std::shared_ptr<CoreEvent>  request_;
            NoCMessageType          message_type_;
            uint16_t                size_;              //! The message size in bits
            uint16_t                src_port_;          //! The source
            uint16_t                dst_port_;          //! The destination
            uint8_t                 noc_network_;       //! The noc network to use
            uint8_t                 class_;             //! The class which represents a priority or VC or both

    };
    
    inline std::ostream& operator<<(std::ostream &str, NoCMessage &mes) {
        str << "Src: " << mes.getSrcPort() << ", Dest: " << mes.getDstPort() << ", Type: " << (int)mes.getType();
        return str;
    }

} // coyote

#endif // __NOC_MESSAGE_HH__
