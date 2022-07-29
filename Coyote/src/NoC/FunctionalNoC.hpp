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


#ifndef __FUNCTIONAL_NOC_H__
#define __FUNCTIONAL_NOC_H__

#include "NoC.hpp"

namespace coyote
{
    class FunctionalNoC : public NoC
    {
        /*!
         * \class coyote::FunctionalNoC
         * \brief Functional model of the NoC with a predefined latency of averaged_packet_latency
         * 
         * It models an idealized network in which each transaction takes a predefined number of cycles
         */
    public:
        /*!
         * \class FunctionalNoCParameterSet
         * \brief Parameters for functional NoC
         */
        class FunctionalNoCParameterSet : public NoCParameterSet
        {
        public:
            //! Constructor for FunctionalNoCParameterSet
            FunctionalNoCParameterSet(sparta::TreeNode* node) :
                NoCParameterSet(node)
            {
            }
            PARAMETER(uint16_t, packet_latency, 30, "The average latency for each packet")
        };

        /*!
         * \brief Constructor for FunctionalNoC
         * \param node The node that represent the NoC and
         * \param params The FunctionalNoC parameter set
         */
        FunctionalNoC(sparta::TreeNode* node, const FunctionalNoCParameterSet* params);

        ~FunctionalNoC() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << std::endl;
        }

        /*!
         * \brief Check the injection queue space in NoC for a packet
         * \param injectedByTile Indicates that the source of the messages is a VAS tile
         * \param mess The packet
         * \return If there is space for the packet on its corresponding injection queue
         */
        virtual bool checkSpaceForPacket(const bool injectedByTile, const std::shared_ptr<NoCMessage> & mess) override;

        /*! \brief Forwards a message from TILE to the actual destination using a predefined latency
         *  \param mess The message to handle
         */
        void handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess) override;

    private:

        /*! \brief Forwards a message from a MCPU to the correct destination using a predefined latency
         *  \param mess The message
         */
        void handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess) override;

        uint16_t    packet_latency_;   //! The latency imposed to each packet

    };

} // coyote

#endif // __FUNCTIONAL_NOC_H__
