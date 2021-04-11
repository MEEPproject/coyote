#include "EventVisitor.hpp"

#include "Event.hpp"   
#include "Sync.hpp"   
#include "Request.hpp"   
#include "CacheRequest.hpp" 
#include "ScratchpadRequest.hpp" 
#include "Fence.hpp"   
#include "Finish.hpp"   
#include "MCPURequest.hpp"
#include "MCPUInstruction.hpp"
#include "InsnLatencyEvent.hpp"

namespace spike_model
{
    void EventVisitor::handle(std::shared_ptr<spike_model::Event> e)
    {
        printf("Using base event handler. This is usually undesired behavior. Might want to override\n");
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::Sync> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Event>(e));
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::Finish> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Sync>(e));
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::Fence> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Sync>(e));
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::Request> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Event>(e));
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::CacheRequest> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Request>(e));
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::InsnLatencyEvent> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Request>(e));
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::ScratchpadRequest> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Request>(e));
    }

    void EventVisitor::handle(std::shared_ptr<spike_model::MCPURequest> e)
    {
        handle(std::dynamic_pointer_cast<spike_model::Event>(e));
    }
            
    void EventVisitor::handle(std::shared_ptr<spike_model::MCPUInstruction> i)
    {
        handle(std::dynamic_pointer_cast<spike_model::Event>(i));
    }
}
