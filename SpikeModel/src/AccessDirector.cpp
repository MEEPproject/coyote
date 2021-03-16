
#include "AccessDirector.hpp"
#include "Tile.hpp"
#include "NoCMessage.hpp"

namespace spike_model
{

    void AccessDirector::putAccess(std::shared_ptr<Request> access)
    {
        access->handle(this);
    }
                
    void AccessDirector::handle(std::shared_ptr<spike_model::CacheRequest> r)
    {
        if(!r->isServiced())
        {
            uint16_t home=calculateHome(r);
            uint16_t bank=calculateBank(r);
            r->setHomeTile(home);
            r->setCacheBank(bank);
            if(r->getHomeTile()==tile->id_)
            {
                //std::cout << "Issuing local l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse  << "\n";
                if(tile->trace_)
                {
                    tile->logger_.logLocalBankRequest(r->getTimestamp(), r->getCoreId(), r->getPC(), r->getCacheBank(), r->getAddress());
                }
                tile->issueLocalRequest_(r, r->getTimestamp()-tile->getClock()->currentCycle()); //MAYBE +1?
            }
            else
            {
                if(tile->trace_)
                {
                    tile->logger_.logRemoteBankRequest(r->getTimestamp(), r->getCoreId(), r->getPC(), r->getHomeTile(), r->getAddress());
                }
                //std::cout << "Issuing remote l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse << "\n";
                tile->issueRemoteRequest_(r, r->getTimestamp()-tile->getClock()->currentCycle());
            }
        }
        else
        {
            if(r->getType()==CacheRequest::AccessType::STORE || r->getType()==CacheRequest::AccessType::WRITEBACK)
            {
                tile->logger_.logMissServiced(tile->getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress());
                tile->request_manager_->notifyAck(r);
            }
            else
            {
                //The ack is for this tile
                if(r->getSourceTile()==tile->id_)
                {
                    //std::cout << "Notifying to manager\n";
                    if(tile->trace_)
                    {
                        tile->logger_.logMissServiced(tile->getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress());
                    }
                    tile->request_manager_->notifyAck(r);
                }
                else
                {
                    //std::cout << "Sending ack to remote\n";
                    if(tile->trace_)
                    {
                        tile->logger_.logTileSendAck(tile->getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getSourceTile(), r->getAddress());
                    }
                    tile->out_port_noc_.send(getDataForwardMessage(r), tile->l2_line_size); 
                }
            }
        }

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
    
    void AccessDirector::setMemoryInfo(uint64_t size_kbs, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile, uint16_t num_tiles, uint64_t num_mcs, uint64_t num_banks_per_mc, uint64_t num_rows_per_bank, uint64_t num_cols_per_bank)
    {
        line_size=line_size;

        block_offset_bits=(uint8_t)(ceil(log2(line_size)));
        tile_bits=(uint8_t)ceil(log2(num_tiles));
        bank_bits=(uint8_t)ceil(log2(banks_per_tile));

        uint64_t s=size_kbs*num_tiles*1024;
        uint64_t num_sets=s/(assoc*line_size);
        set_bits=(uint8_t)ceil(log2(num_sets));
        tag_bits=64-(set_bits+block_offset_bits);

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

    std::shared_ptr<NoCMessage> AccessDirector::getRemoteL2RequestMessage(std::shared_ptr<CacheRequest> req)
    {
        return std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_REQUEST, address_size);
    }
            
    std::shared_ptr<NoCMessage> AccessDirector::getMemoryRequestMessage(std::shared_ptr<CacheRequest> req)
    {
        //This should go to the memory controller
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

        if(req->getType()==CacheRequest::AccessType::STORE)
        {
            size=req->getSize();
        }
        else if(req->getType()==CacheRequest::AccessType::WRITEBACK)
        {
            size=line_size;
        }         
        
        return std::make_shared<NoCMessage>(req, NoCMessageType::MEMORY_REQUEST, size);     
    }     
     
    std::shared_ptr<NoCMessage> AccessDirector::getDataForwardMessage(std::shared_ptr<CacheRequest> req)     
    {         
        return std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_ACK, line_size);     
    }

}
