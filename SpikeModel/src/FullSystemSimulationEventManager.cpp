
#include "FullSystemSimulationEventManager.hpp"
#include "NoC/NoCMessageType.hpp"

namespace spike_model
{
    FullSystemSimulationEventManager::FullSystemSimulationEventManager(std::vector<Tile *> tiles, uint16_t cores_per_tile)
    {
        tiles_=tiles;
        cores_per_tile_=cores_per_tile;
    }

    void FullSystemSimulationEventManager::notifyAck(const std::shared_ptr<Event>& req)
    {
        serviced_requests_.addRequest(req);
    }

    bool FullSystemSimulationEventManager::hasServicedRequest()
    {
        return serviced_requests_.hasRequest();
    }

    std::shared_ptr<Event> FullSystemSimulationEventManager::getServicedRequest()
    {
        return serviced_requests_.getRequest();
    }

    void FullSystemSimulationEventManager::putEvent(const std::shared_ptr<Event>& ev)
    {
//        printf("In handle\n");
        ev->handle(this);
    }


    void FullSystemSimulationEventManager::handle(std::shared_ptr<spike_model::CoreEvent> r)
    {
        uint16_t source=r->getCoreId()/cores_per_tile_;
        r->setSourceTile(source);
        tiles_[source]->putEvent(r);
    }

    void FullSystemSimulationEventManager::scheduleArbiter()
    {
        for(auto itr = tiles_.begin(); itr != tiles_.end(); itr++)
        {
            (*itr)->getArbiter()->submitToNoC();
            (*itr)->getArbiter()->submitToL2();
        }
    }

    bool FullSystemSimulationEventManager::hasArbiterQueueFreeSlot(uint16_t core)
    {
        bool ret = false;
        for(auto itr = tiles_.begin(); itr != tiles_.end(); itr++)
        {
            ret = ret || (*itr)->getArbiter()->hasArbiterQueueFreeSlot((*itr)->getId(), core);
        }
        return ret;
    }

    bool FullSystemSimulationEventManager::hasMsgInArbiter()
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
