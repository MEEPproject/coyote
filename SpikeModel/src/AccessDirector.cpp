
#include "AccessDirector.hpp"
#include "Tile.hpp"
#include "NoC/NoCMessage.hpp"
#include "ScratchpadRequest.hpp"

namespace spike_model
{

    void AccessDirector::putAccess(std::shared_ptr<Request> access)
    {
        access->handle(this);
    }
                
    void AccessDirector::handle(std::shared_ptr<spike_model::CacheRequest> r)
    {
        if(r->memoryAck())
        {
            r->setMemoryAck(false);
            tile->issueBankAck_(r);
        }
        else if(!r->isServiced())
        {
            uint64_t address=r->getAddress();
            uint64_t memory_controller=0;
            if(mc_mask!=0)
            {
                memory_controller=(address >> mc_shift) & mc_mask;
            }
            
            r->setMemoryController(memory_controller);
            
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

                uint64_t lapse=0;
                if(r->getTimestamp()+1>tile->getClock()->currentCycle())
                {
                    lapse=(r->getTimestamp()+1)-tile->getClock()->currentCycle(); //Requests coming from spike have to account for clock synchronization
                }
                tile->issueLocalRequest_(r, lapse);
            }
            else
            {
                if(tile->trace_)
                {
                    tile->logger_.logRemoteBankRequest(r->getTimestamp(), r->getCoreId(), r->getPC(), r->getHomeTile(), r->getAddress());
                }
                //std::cout << "Issuing remote l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse << "\n";
                tile->issueRemoteRequest_(r, (r->getTimestamp()+1)-tile->getClock()->currentCycle());
            }
        }
        else
        {
            if(r->getType()==CacheRequest::AccessType::STORE || r->getType()==CacheRequest::AccessType::WRITEBACK)
            {
                if(tile->trace_)
                {
                    tile->logger_.logMissServiced(tile->getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress());
                }
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
                    tile->getArbiter()->submit(getDataForwardMessage(r), true, r->getCoreId());
                }
            }
        }
    }
    
    // This function is used to handle scratchpad requests both before and after they access the scratchpad.
    // The scratchpad DOES NOT perform any checks on sizes or address ranges. The MCPU should be clever enough
    // to not request more size than the L2 size or not read/write to addresses that have not been allocated
    // for scratchpad.
    void AccessDirector::handle(std::shared_ptr<spike_model::ScratchpadRequest> r)
    {
        uint64_t request_size=r->getSize();
        switch(r->getCommand())
        {
            case ScratchpadRequest::ScratchpadCommand::ALLOCATE:
            {
                if(!r->isServiced())
                {
                    // If there is not enough available scratchpad size, disable the necessary ways to
                    // obtain it
                    if(scratchpad_available_size<request_size)
                    {
                        uint64_t ways_to_disable=ceil(request_size/way_size);
                        r->setSize(ways_to_disable); //THIS IS A NASTY TRICK

                        //Ways are disabled in every bank
                        for(int i=0; i<tile->num_l2_banks_;i++)
                        {
                            tile->out_ports_l2_reqs_[i]->send(r, 1); //disableWay(ways_to_disable)
                        }
                        pending_scratchpad_management_ops[r]=tile->num_l2_banks_;

                        scratchpad_ways=scratchpad_ways+ways_to_disable;
                        scratchpad_available_size=scratchpad_available_size+(way_size*ways_to_disable);
                    }
                    else //If ways have been disabled the ACK will be sent when all banks finish disabling
                    {
                        //SEND ACK TO MCPU
                        tile->getArbiter()->submit(getScratchpadAckMessage(r), true, r->getCoreId());
                    }
                    scratchpad_available_size=scratchpad_available_size-request_size;

                }
                else
                {
                    pending_scratchpad_management_ops[r]=pending_scratchpad_management_ops[r]-1;

                    if(pending_scratchpad_management_ops[r]==0)
                    {
                        //SEND ACK TO MCPU
                        tile->getArbiter()->submit(getScratchpadAckMessage(r), true, r->getCoreId());
                        pending_scratchpad_management_ops.erase(r);
                    }
                }
                break;
            }
            case ScratchpadRequest::ScratchpadCommand::FREE:
            {
                if(!r->isServiced())
                {
                    scratchpad_available_size=scratchpad_available_size+request_size;

                    uint64_t ways_to_enable=0;

                    if(scratchpad_available_size==way_size*scratchpad_ways)//Scratchpad completely unused. Disable it
                    {
                        ways_to_enable=scratchpad_ways;
                    }
                    else if(scratchpad_available_size>1.5*way_size) //A way is enabled (for cache) if more than a way and a half is available for scratchpad
                    {
                        ways_to_enable=round(scratchpad_available_size/(1.5*way_size));
                    }

                    if(ways_to_enable!=0)
                    {
                        r->setSize(ways_to_enable); //THIS IS A NASTY TRICK
                        
                        //Ways are enabled in every bank
                        for(int i=0; i<tile->num_l2_banks_;i++)
                        {
                            tile->out_ports_l2_reqs_[i]->send(r, 1); //enableWays(ways_to_enable)
                        }
                        pending_scratchpad_management_ops[r]=tile->num_l2_banks_;

                        scratchpad_ways=scratchpad_ways-ways_to_enable;
                        scratchpad_available_size=scratchpad_available_size-(ways_to_enable*way_size);
                    }
                    else //If ways have been enabled the ACK will be sent when all banks finish enabling
                    {
                        //SEND ACK TO MCPU
                        tile->getArbiter()->submit(getScratchpadAckMessage(r), true, r->getCoreId());
                    }
                }
                else
                {
                    pending_scratchpad_management_ops[r]=pending_scratchpad_management_ops[r]-1;

                    if(pending_scratchpad_management_ops[r]==0)
                    {
                        //SEND ACK TO MCPU
                        tile->getArbiter()->submit(getScratchpadAckMessage(r), true, r->getCoreId());
                        pending_scratchpad_management_ops.erase(r);
                    }
                }
                break;
            }
            case ScratchpadRequest::ScratchpadCommand::READ:
            {
                if(!r->isServiced())
                {
                    uint16_t b=calculateBank(r);
                    r->setCacheBank(b);
                    tile->issueLocalRequest_(r, 1);
                }
                else
                {
                    //Send ACK to MCPU
                    tile->getArbiter()->submit(getScratchpadAckMessage(r), true, r->getCoreId());
                }
                break;
            }
            case ScratchpadRequest::ScratchpadCommand::WRITE:
            {
                if(!r->isServiced())
                {
                    uint16_t b=calculateBank(r);
                    r->setCacheBank(b);
                    tile->issueLocalRequest_(r, 1);
                }
                else if(r->isOperandReady())
                {
                    //Send "activate!" to Core
                    tile->request_manager_->notifyAck(r);
                }
                break;
            }
        }
    }
    
    uint8_t AccessDirector::nextPowerOf2(uint64_t v)
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
    
    void AccessDirector::setMemoryInfo(uint64_t size_kbs, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile, uint16_t num_tiles, uint64_t num_mcs, AddressMappingPolicy address_mapping_policy)
    {
        this->line_size=line_size;

        block_offset_bits=(uint8_t)(ceil(log2(line_size)));
        tile_bits=(uint8_t)ceil(log2(num_tiles));
        bank_bits=(uint8_t)ceil(log2(banks_per_tile));

        uint64_t s=size_kbs*num_tiles*1024;
        uint64_t num_sets=s/(assoc*line_size);
        set_bits=(uint8_t)ceil(log2(num_sets));
        tag_bits=64-(set_bits+block_offset_bits);
        num_ways=assoc;
        way_size=(size_kbs/num_ways)*1024;

        switch(address_mapping_policy)
        {
            case AddressMappingPolicy::OPEN_PAGE:
                mc_shift=ceil(log2(line_size));
                mc_mask=nextPowerOf2(num_mcs)-1;
                break;

                
            case AddressMappingPolicy::CLOSE_PAGE:
                mc_shift=ceil(log2(line_size));
                mc_mask=nextPowerOf2(num_mcs)-1;
                break;
        }


    }

    std::shared_ptr<NoCMessage> AccessDirector::getRemoteL2RequestMessage(std::shared_ptr<CacheRequest> req)
    {
        return std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_REQUEST, address_size, req->getSourceTile(), req->getHomeTile());
    }
            
    std::shared_ptr<NoCMessage> AccessDirector::getMemoryRequestMessage(std::shared_ptr<CacheRequest> req)
    {
        //This should go to the memory controller

        uint32_t size=address_size;
        NoCMessageType type=NoCMessageType::MEMORY_REQUEST_LOAD;

        if(req->getType()==CacheRequest::AccessType::STORE)
        {
            size=req->getSize();
            type=NoCMessageType::MEMORY_REQUEST_STORE;
        }
        else if(req->getType()==CacheRequest::AccessType::WRITEBACK)
        {
            uint64_t address=req->getAddress();
            
            uint64_t memory_controller=0;
            if(mc_mask!=0)
            {
                memory_controller=(address >> mc_shift) & mc_mask;
            }

            req->setMemoryController(memory_controller);
            //Adds missing info to WRITEBACKS that was not available in the CacheBank 
            size=line_size;
            req->setSourceTile(tile->id_);
            uint16_t home=calculateHome(req);
            req->setHomeTile(home);
            type=NoCMessageType::MEMORY_REQUEST_WB;
        }

        return std::make_shared<NoCMessage>(req, type, size, req->getHomeTile(), req->getMemoryController());
    }
     
    std::shared_ptr<NoCMessage> AccessDirector::getDataForwardMessage(std::shared_ptr<CacheRequest> req)     
    {         
        return std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_ACK, line_size, req->getHomeTile(), req->getSourceTile());
    }
    
    std::shared_ptr<NoCMessage> AccessDirector::getScratchpadAckMessage(std::shared_ptr<ScratchpadRequest> req)
    {
        return std::make_shared<NoCMessage>(req, NoCMessageType::SCRATCHPAD_ACK, line_size, tile->id_, 0);
    }

    uint16_t AccessDirector::calculateBank(std::shared_ptr<spike_model::ScratchpadRequest> r)
    {
        uint16_t destination=0;
        
        if(bank_bits>0) //If more than one bank
        {
            uint8_t left=tag_bits;
            uint8_t right=block_offset_bits;
            switch(scratchpad_data_mapping_policy_)
            {
                case CacheDataMappingPolicy::SET_INTERLEAVING:
                    left+=set_bits-bank_bits;
                    break;
                case CacheDataMappingPolicy::PAGE_TO_BANK:
                    right+=set_bits-bank_bits;
                    break;
            }
            destination=(r->getAddress() << left) >> (left+right);
        }
        return destination;
    }

}
