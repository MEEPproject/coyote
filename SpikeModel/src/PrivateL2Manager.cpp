#include "PrivateL2Manager.hpp"

namespace spike_model
{
    PrivateL2Manager::PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile) : RequestManager(tiles, cores_per_tile) {}
    
    PrivateL2Manager::PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, DataMappingPolicy b) : RequestManager(tiles, cores_per_tile, b) {}

    void PrivateL2Manager::putRequest(std::shared_ptr<L2Request> req, uint64_t lapse)
    {
        RequestManager::putRequest(req, lapse);
        uint16_t home=req->getSourceTile();
        req->setHomeTile(home);
        //Send
        tiles_[req->getSourceTile()]->putRequest_(req, lapse);
    }
}
