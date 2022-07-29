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


#ifndef __FULL_SYSTEM_SIMULATION_EVENT_MANAGER_HH__
#define __FULL_SYSTEM_SIMULATION_EVENT_MANAGER_HH__

#include <memory>
#include "ServicedRequests.hpp"
#include "Tile.hpp"
#include "NoC/NoCMessage.hpp"
#include "AddressMappingPolicy.hpp"
#include "Event.hpp"
#include "MemoryTile/MCPUSetVVL.hpp"
#include "SimulationEntryPoint.hpp"

class Coyote; //Forward declaration
class ExecutionDrivenSimulationOrchestrator;

namespace coyote
{
    class Tile; //Forward declaration

    class FullSystemSimulationEventManager : public coyote::EventVisitor, public coyote::SimulationEntryPoint
    {
        friend class ::ExecutionDrivenSimulationOrchestrator;

        using coyote::EventVisitor::handle; //This prevents the compiler from warning on overloading 

        /**
         * \class coyote::FullSystemSimulationEventManager
         *
         * \brief FullSystemSimulationEventManager is the main interface between Spike and Sparta.
         *
         * Instances of ExecutionDrivenSimulationOrchestrator use an instance of FullSystemSimulationEventManager to forward a Request 
         * to Sparta and check for its completion. The FullSystemSimulationEventManager also holds information regarding
         * the memory hiwerarchy of the modelled architecture, to update the data of requests.
         *
         */

        friend class ::Coyote;

        public:
 
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             */
            FullSystemSimulationEventManager(std::vector<Tile *> tiles, uint16_t cores_per_tile);
            
            /*!
             * \brief Submit an event.
             * \param ev The event to submit.
             */
            virtual void putEvent(const std::shared_ptr<Event> &) override;
            
            /*!
             * \brief Notify the completion of request
             * \param req The request that has been completed
             */
            void notifyAck(const std::shared_ptr<Event>& req);
        

            /*!
             * \brief Query for the availability of serviced requests
             * \return true if there is any serviced request
             */
            bool hasServicedRequest();

            /*!
             * \brief Get a serviced request
             * \return A request that has been serviced
             */
            std::shared_ptr<Event> getServicedRequest();

            void scheduleArbiter();
            bool hasMsgInArbiter();
            bool hasArbiterQueueFreeSlot(uint16_t core);

        protected:
            std::vector<Tile *> tiles_;
            uint16_t cores_per_tile_;
           
        
        private:
            ServicedRequests serviced_requests_;
            
            
            /*!
             * \brief Set the storage for the requests that have been acknowledged 
             * \param s The storage for acknowledged requests
             * \note This method is called through friending by Coyote
             */
            void setServicedRequestsStorage(ServicedRequests& s)
            {
                serviced_requests_=s;
//                std::cout << s.hasRequest();
            }
             
             /*!
             * \brief Handles a core event
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::CoreEvent> r) override;
            
    };
}
#endif
