
#include "RequestManagerIF.hpp"
#include "NoCMessageType.hpp"

namespace spike_model
{
    RequestManagerIF::RequestManagerIF(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy)
    {
        tiles_=tiles;
        cores_per_tile_=cores_per_tile;
        bank_data_mapping_policy_=CacheDataMappingPolicy::PAGE_TO_BANK;
        address_mapping_policy_=address_mapping_policy;
    }
    
    RequestManagerIF::RequestManagerIF(std::vector<Tile *> tiles, uint16_t cores_per_tile, spike_model::AddressMappingPolicy address_mapping_policy, CacheDataMappingPolicy b)
    {
        tiles_=tiles;
        cores_per_tile_=cores_per_tile;
        bank_data_mapping_policy_=b;
        address_mapping_policy_=address_mapping_policy;
    }

    void RequestManagerIF::notifyAck(const std::shared_ptr<Request>& req)
    {
        serviced_requests_.addRequest(req);
    }
    
    bool RequestManagerIF::hasServicedRequest()
    {
        return serviced_requests_.hasRequest();
    }

    std::shared_ptr<Request> RequestManagerIF::getServicedRequest()
    {
        return serviced_requests_.getRequest();
    }
            
    uint8_t nextPowerOf2(uint64_t v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v++;
        return v;
    }

    void RequestManagerIF::setMemoryInfo(uint64_t size_kbs, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile, uint64_t num_mcs, uint64_t num_banks_per_mc, uint64_t num_rows_per_bank, uint64_t num_cols_per_bank)
    {
        line_size=line_size;

        block_offset_bits=(uint8_t)(ceil(log2(line_size)));
        tile_bits=(uint8_t)ceil(log2(tiles_.size()));
        bank_bits=(uint8_t)ceil(log2(banks_per_tile));

        uint64_t s=size_kbs*tiles_.size()*1024;
        uint64_t num_sets=s/(assoc*line_size);
        set_bits=(uint8_t)ceil(log2(num_sets));
        tag_bits=64-(set_bits+block_offset_bits);

        for(size_t i=0;i<tiles_.size();i++)
        {
            tiles_[i]->setL2BankInfo(size_kbs/tiles_.size(), assoc, line_size);
        }

        switch(address_mapping_policy_)
        {
            case AddressMappingPolicy::OPEN_PAGE:
                mc_shift=ceil(log2(line_size));
                col_shift=mc_shift+ceil(log2(num_cols_per_bank));
                bank_shift=col_shift+ceil(log2(num_banks_per_mc));
                rank_shift=bank_shift+0;
                row_shift=rank_shift+ceil(log2(num_rows_per_bank));
                break;

                
            case AddressMappingPolicy::CLOSE_PAGE:
                mc_shift=ceil(log2(line_size));
                bank_shift=mc_shift+ceil(log2(num_banks_per_mc));
                rank_shift=bank_shift+0;
                col_shift=rank_shift+ceil(log2(num_cols_per_bank));
                row_shift=col_shift+ceil(log2(num_rows_per_bank));
                break;
        }

        mc_mask=nextPowerOf2(num_mcs)-1;
        rank_mask=0;
        bank_mask=nextPowerOf2(num_banks_per_mc)-1;
        row_mask=nextPowerOf2(num_rows_per_bank)-1;
        col_mask=nextPowerOf2(num_cols_per_bank)-1;

    }

    void RequestManagerIF::putRequest(std::shared_ptr<Request> req, uint64_t lapse)
    {
        uint16_t source=req->getCoreId()/cores_per_tile_;
        uint16_t bank=req->calculateHome(bank_data_mapping_policy_, tag_bits, block_offset_bits, set_bits, bank_bits);
        req->setSourceTile(source);
        req->setCacheBank(bank);
        req->calculateLineAddress(block_offset_bits);
    }
            
    std::shared_ptr<NoCMessage> RequestManagerIF::getRemoteL2RequestMessage(std::shared_ptr<Request> req)
    {
        return std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_REQUEST, address_size);
    }
            
    std::shared_ptr<NoCMessage> RequestManagerIF::getMemoryRequestMessage(std::shared_ptr<Request> req)
    {
        uint64_t address=req->getAddress();
        
        uint64_t memory_controller=0;
        if(mc_mask!=0)
        {
            memory_controller=(address >> mc_shift) & mc_mask;
        }

        uint64_t rank=0;
        if(rank_mask!=0)
        {
            rank=(address >> rank_shift) & rank_mask;
        }

        uint64_t bank=0;
        if(bank_mask!=0)
        {
            bank=(address >> bank_shift) & bank_mask;
        }

        uint64_t row=0;
        if(row_mask!=0)
        {
            row=(address >> row_shift) & row_mask;
        }

        uint64_t col=0;
        if(col_mask!=0)
        {
            col=(address >> col_shift) & col_mask;
        }

        req->setMemoryAccessInfo(memory_controller, rank, bank, row, col);

        uint32_t size=address_size;

        if(req->getType()==Request::AccessType::STORE)
        {
            size=req->getSize();
        }
        else if(req->getType()==Request::AccessType::WRITEBACK)
        {
            size=line_size;
        }

        return std::make_shared<NoCMessage>(req, NoCMessageType::MEMORY_REQUEST, size);
    }

    std::shared_ptr<NoCMessage> RequestManagerIF::getMemoryReplyMessage(std::shared_ptr<Request> req)
    {
        return std::make_shared<NoCMessage>(req, NoCMessageType::MEMORY_ACK, line_size);
    }

    std::shared_ptr<NoCMessage> RequestManagerIF::getDataForwardMessage(std::shared_ptr<Request> req)
    {
        return std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_ACK, line_size);
    }
            
}
