
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
        uint16_t source=req->getCoreId()/cores_per_tile_;
        req->setSourceTile(source);
        tiles_[source]->putRequest_(req);
    }

    void EventManager::scheduleArbiter(uint64_t current_cycle)
    {
        for(auto itr = tiles_.begin(); itr != tiles_.end(); itr++)
        {
            (*itr)->getArbiter()->submitToNoC(current_cycle);
        }
    }

    bool EventManager::hasNoCMsgInNetwork()
    {
        for(auto itr = tiles_.begin(); itr != tiles_.end(); itr++)
        {
            if((*itr)->getArbiter()->hasNoCMsgInNetwork())
                return true;
        }
        return false;
    }
}
