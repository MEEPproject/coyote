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
