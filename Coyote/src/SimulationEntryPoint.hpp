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

#ifndef __SIMULATION_ENTRY_POINT_HH__
#define __SIMULATION_ENTRY_POINT_HH__

class TraceDrivenSimulationOrchestrator; //Forward declaration

namespace coyote
{
    class SimulationEntryPoint
    { 
        friend class FullSystemSimulationEventManager;
        friend class ::TraceDrivenSimulationOrchestrator;

        /**
         * \class coyote::SimulationEntryPoint
         *
         * \brief SimulationEntryPoint objects can be input events as the starting point for simulation
         *
         */

        protected:
            /*!
             * \brief Submit an event.
             * \param ev The event to submit.
             * \note This method bypasses ports and timing and inputs the event straightaway. Thus, it should be used with care. 
             * For this reason, it is protected and to be used externally through friending. An example of adequate uses of this method can
             * be found in classes coyote::FullSystemSimulationEventManager and coyote::TraceDrivenSimulationOrchestrator.
             */ 
            virtual void putEvent(const std::shared_ptr<Event> &)=0;
    };
}
#endif
