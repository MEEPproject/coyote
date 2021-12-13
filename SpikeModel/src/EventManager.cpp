
#include "EventManager.hpp"
#include "NoC/NoCMessageType.hpp"

namespace spike_model
{
    EventManager::EventManager(std::vector<Tile *> tiles, uint16_t cores_per_tile)
    {
        tiles_=tiles;
        cores_per_tile_=cores_per_tile;
    }

    void EventManager::notifyAck(const std::shared_ptr<Event>& req)
    {
        serviced_requests_.addRequest(req);
    }

    bool EventManager::hasServicedRequest()
    {
        return serviced_requests_.hasRequest();
    }

    std::shared_ptr<Event> EventManager::getServicedRequest()
    {
        return serviced_requests_.getRequest();
    }

    void EventManager::putRequest(std::shared_ptr<Event> req)
    {
//        printf("In handle\n");
        req->handle(this);
    }


    void EventManager::handle(std::shared_ptr<spike_model::CoreEvent> r)
    {
        uint16_t source=r->getCoreId()/cores_per_tile_;
        r->setSourceTile(source);
        tiles_[source]->putRequest_(r);
    }

    void EventManager::scheduleArbiter()
    {
        for(auto itr = tiles_.begin(); itr != tiles_.end(); itr++)
        {
            (*itr)->getArbiter()->submitToNoC();
            (*itr)->getArbiter()->submitToL2();
        }
    }

    bool EventManager::hasArbiterQueueFreeSlot(uint16_t core)
    {
        bool ret = false;
        for(auto itr = tiles_.begin(); itr != tiles_.end(); itr++)
        {
            ret = ret || (*itr)->getArbiter()->hasArbiterQueueFreeSlot((*itr)->getId(), core);
        }
        return ret;
    }

    bool EventManager::hasMsgInArbiter()
    {
        for(auto itr = tiles_.begin(); itr != tiles_.end(); itr++)
        {
            if((*itr)->getArbiter()->hasNoCMsgInNetwork())
                return true;
            else if((*itr)->getArbiter()->hasCacheRequestInNetwork())
                return true;
        }
        return false;
    }
}
