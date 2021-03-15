#include <SharedL2Manager.hpp>

namespace spike_model
{
    SharedL2Manager::SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy) : RequestManagerIF(tiles, cores_per_tile, address_mapping_policy)
    {
        tile_data_mapping_policy=CacheDataMappingPolicy::SET_INTERLEAVING;
    }
    
    SharedL2Manager::SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b, CacheDataMappingPolicy d) : RequestManagerIF(tiles, cores_per_tile, address_mapping_policy, b)
    {
        tile_data_mapping_policy=d;
    }


    void SharedL2Manager::putRequest(std::shared_ptr<CacheRequest> req)
    {
        RequestManagerIF::putRequest(req);
        uint16_t home=req->calculateHome(tile_data_mapping_policy, tag_bits, block_offset_bits, set_bits, tile_bits);
        req->setHomeTile(home);
        //Send
        tiles_[req->getSourceTile()]->putRequest_(req);
    }
}
