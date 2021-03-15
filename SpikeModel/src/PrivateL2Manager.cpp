#include "PrivateL2Manager.hpp"

namespace spike_model
{
    PrivateL2Manager::PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy) : RequestManagerIF(tiles, cores_per_tile, address_mapping_policy) {}
    
    PrivateL2Manager::PrivateL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b) : RequestManagerIF(tiles, cores_per_tile, address_mapping_policy, b) {}

    void PrivateL2Manager::putRequest(std::shared_ptr<CacheRequest> req)
    {
        RequestManagerIF::putRequest(req);
        uint16_t home=req->getSourceTile();
        req->setHomeTile(home);
        //Send
        tiles_[req->getSourceTile()]->putRequest_(req);
    }
}
