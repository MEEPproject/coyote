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

#include "EventVisitor.hpp"

#include "Event.hpp"   
#include "Sync.hpp"   
#include "Request.hpp"   
#include "CacheRequest.hpp" 
#include "ScratchpadRequest.hpp" 
#include "Fence.hpp"   
#include "Finish.hpp"   
#include "MemoryTile/MCPUSetVVL.hpp"
#include "MemoryTile/MCPUInstruction.hpp"
#include "InsnLatencyEvent.hpp"
#include "VectorWaitingForScalarStore.hpp"

namespace coyote
{
    void EventVisitor::handle(std::shared_ptr<coyote::Event> e)
    {
        printf("Using base event handler. This is usually undesired behavior. Might want to override\n");
    }
    
    void EventVisitor::handle(std::shared_ptr<coyote::CoreEvent> e)
    {
        handle(std::dynamic_pointer_cast<coyote::Event>(e));
    }
    
    void EventVisitor::handle(std::shared_ptr<coyote::RegisterEvent> e)
    {
        handle(std::dynamic_pointer_cast<coyote::CoreEvent>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::Sync> e)
    {
        handle(std::dynamic_pointer_cast<coyote::CoreEvent>(e));
    }
    
    void EventVisitor::handle(std::shared_ptr<coyote::VectorWaitingForScalarStore> e)
    {
        handle(std::dynamic_pointer_cast<coyote::Sync>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::Finish> e)
    {
        handle(std::dynamic_pointer_cast<coyote::Sync>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::Fence> e)
    {
        handle(std::dynamic_pointer_cast<coyote::Sync>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::Request> e)
    {
        handle(std::dynamic_pointer_cast<coyote::RegisterEvent>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::CacheRequest> e)
    {
        handle(std::dynamic_pointer_cast<coyote::Request>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::InsnLatencyEvent> e)
    {
        handle(std::dynamic_pointer_cast<coyote::RegisterEvent>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::ScratchpadRequest> e)
    {
        handle(std::dynamic_pointer_cast<coyote::Request>(e));
    }

    void EventVisitor::handle(std::shared_ptr<coyote::MCPUSetVVL> e)
    {
        handle(std::dynamic_pointer_cast<coyote::CoreEvent>(e));
    }
            
    void EventVisitor::handle(std::shared_ptr<coyote::MCPUInstruction> i)
    {
        handle(std::dynamic_pointer_cast<coyote::RegisterEvent>(i));
    }
}
