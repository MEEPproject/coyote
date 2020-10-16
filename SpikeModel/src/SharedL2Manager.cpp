#include <SharedL2Manager.hpp>

namespace spike_model
{
    SharedL2Manager::SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile) : RequestManager(tiles, cores_per_tile)
    {
        tile_data_mapping_policy=DataMappingPolicy::SET_INTERLEAVING;
    }
    
    SharedL2Manager::SharedL2Manager(std::vector<Tile *> tiles, uint16_t cores_per_tile, DataMappingPolicy b, DataMappingPolicy d) : RequestManager(tiles, cores_per_tile, b)
    {
        tile_data_mapping_policy=d;
    }


    void SharedL2Manager::putRequest(std::shared_ptr<L2Request> req, uint64_t lapse)
    {
        RequestManager::putRequest(req, lapse);
        uint16_t home=req->calculateHome(tile_data_mapping_policy, tag_bits, block_offset_bits, set_bits, tile_bits);
        req->setHomeTile(home);
        //Send
        tiles_[req->getSourceTile()]->putRequest_(req, lapse);
    }
}
