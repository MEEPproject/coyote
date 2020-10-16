
#include <RequestManager.hpp>

namespace spike_model
{
    RequestManager::RequestManager(std::vector<Tile *> tiles, uint16_t cores_per_tile)
    {
        tiles_=tiles;
        cores_per_tile_=cores_per_tile;
        bank_data_mapping_policy=DataMappingPolicy::PAGE_TO_BANK;
    }
    
    RequestManager::RequestManager(std::vector<Tile *> tiles, uint16_t cores_per_tile, DataMappingPolicy b)
    {
        tiles_=tiles;
        cores_per_tile_=cores_per_tile;
        bank_data_mapping_policy=b;
    }

    void RequestManager::notifyAck(const std::shared_ptr<L2Request>& req)
    {
        serviced_requests.addRequest(req);
    }
    
    bool RequestManager::hasServicedRequest()
    {
        return serviced_requests.hasRequest();
    }

    std::shared_ptr<L2Request> RequestManager::getServicedRequest()
    {
        return serviced_requests.getRequest();
    }
            
    void RequestManager::setL2TileInfo(uint64_t size_kbs, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile)
    {
        std::cout << "The result is " << ceil(log2(line_size)) << "\n";
        
        
        
        
        uint8_t res=(uint8_t)(ceil(log2(line_size)));
        block_offset_bits=res;
        tile_bits=(uint8_t)ceil(log2(tiles_.size()));
        bank_bits=(uint8_t)ceil(log2(banks_per_tile));

        uint64_t s=size_kbs*tiles_.size()*1024;
        uint64_t num_sets=s/(assoc*line_size);
        set_bits=(uint8_t)ceil(log2(num_sets));
        tag_bits=64-(set_bits+block_offset_bits);
        std::cout << "There are " << unsigned(tile_bits) << " bits for banks and " << unsigned(set_bits) << " bits for sets\n";

        for(size_t i=0;i<tiles_.size();i++)
        {
            tiles_[i]->setL2BankInfo(size_kbs/tiles_.size(), assoc, line_size);
        }
    }

    void RequestManager::putRequest(std::shared_ptr<L2Request> req, uint64_t lapse)
    {
        uint16_t source=req->getCoreId()/cores_per_tile_;
        uint16_t bank=req->calculateHome(bank_data_mapping_policy, tag_bits, block_offset_bits, set_bits, bank_bits);
        req->setSourceTile(source);
        req->setBank(bank);
    }
}
