// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 

#include "AccessDirector.hpp"
#include "Tile.hpp"
#include "NoC/NoCMessage.hpp"
#include "ScratchpadRequest.hpp"

namespace coyote
{

    void AccessDirector::putAccess(std::shared_ptr<Request> access)
    {
        access->handle(this);
    }
                
    void AccessDirector::handle(std::shared_ptr<coyote::CacheRequest> r)
    {
        if(r->memoryAck() && !r->getBypassL2())
        {
            r->setMemoryAck(false);
            r->setServiced();
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
           
            if(!r->getBypassL2())
            {
                uint16_t home=calculateHome(r);
                uint16_t bank=calculateBank(r);
                r->setHomeTile(home);
                r->setCacheBank(bank);
                if(r->getHomeTile()==tile->id_)
                {
                    if(r->getHomeTile()==r->getSourceTile())
                    {
                        tile->count_local_requests_++;
                    }
                    else
                    {
                        tile->count_remote_requests_++;
                    }

                    uint64_t lapse=0;
                    if(r->getTimestamp()+1>tile->getClock()->currentCycle())
                    {
                        lapse=(r->getTimestamp())-tile->getClock()->currentCycle(); //Requests coming from spike have to account for clock synchronization
                    }
                    //std::cout << "Issuing local l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse  << "\n";
                    if(tile->trace_)
                    {
                        tile->logger_->logLocalBankRequest(tile->getClock()->currentCycle()+lapse, r->getCoreId(), r->getPC(), r->getCacheBank(), r->getAddress());
                    }
                    tile->issueLocalRequest_(r, lapse);
                }
                else
                {
                    if(tile->trace_)
                    {
                        tile->logger_->logRemoteBankRequest(r->getTimestamp(), r->getCoreId(), r->getPC(), r->getHomeTile(), r->getAddress());
                    }
                    //std::cout << "Issuing remote l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse << "\n";
                    tile->issueRemoteRequest_(r, (r->getTimestamp()+1)-tile->getClock()->currentCycle());
                }
            }
            else
            {
                //printf("Bypassing\n");
                tile->issueMemoryControllerRequest_(r, true);
            }
        }
        else
        {
            if(r->getType()==CacheRequest::AccessType::STORE || r->getType()==CacheRequest::AccessType::WRITEBACK)
            {
                if(tile->trace_)
                {
                    tile->logger_->logMissServiced(tile->getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress());
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
                        tile->logger_->logMissServiced(tile->getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress());
                    }
                    tile->request_manager_->notifyAck(r); //This should go through the arbiter
                }
                else
                {
                    //std::cout << "Sending ack to remote\n";
                    if(tile->trace_)
                    {
                        tile->logger_->logTileSendAck(tile->getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getSourceTile(), r->getAddress());
                    }
	            std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                    msg->msg = getDataForwardMessage(r);
                    msg->is_core = true;
                    msg->id = r->getCoreId();
                    msg->type = coyote::MessageType::NOC_MSG;
                    tile->out_port_arbiter_.send(msg, 0);
                }
            }
        }
    }
    
    // This function is used to handle scratchpad requests both before and after they access the scratchpad.
    // The scratchpad DOES NOT perform any checks on sizes or address ranges. The MCPU should be clever enough
    // to not request more size than the L2 size or not read/write to addresses that have not been allocated
    // for scratchpad.
    void AccessDirector::handle(std::shared_ptr<coyote::ScratchpadRequest> r)
    {
        switch(r->getCommand())
        {
            case ScratchpadRequest::ScratchpadCommand::ALLOCATE:
            {
                /*if(!r->isServiced())
                {
                    // If there is not enough available scratchpad size, disable the necessary ways to
                    // obtain it
                    if(scratchpad_available_size<request_size)
                    {
                        //uint64_t ways_to_disable=ceil(1.0*request_size/way_size);
                        uint64_t ways_to_disable=8;//ceil(1.0*request_size/way_size);  //HARDCODED DISABLING OF 8 WAYS
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
	                std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                        msg->msg = getScratchpadAckMessage(r);
                        msg->is_core = true;
                        msg->id = r->getCoreId();
                        msg->type = coyote::MessageType::NOC_MSG;
                        tile->out_port_arbiter_.send(msg, 0);
                    }
                    scratchpad_available_size=scratchpad_available_size-request_size;

                }
                else
                {
                    pending_scratchpad_management_ops[r]=pending_scratchpad_management_ops[r]-1;

                    if(pending_scratchpad_management_ops[r]==0)
                    {
                        //SEND ACK TO MCPU
	                std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                        msg->msg = getScratchpadAckMessage(r);
                        msg->is_core = true;
                        msg->id = r->getCoreId();
                        msg->type = coyote::MessageType::NOC_MSG;
                        tile->out_port_arbiter_.send(msg, 0);
                        pending_scratchpad_management_ops.erase(r);
                    }
                }*/
	                std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                        msg->msg = getScratchpadAckMessage(r);
                        msg->is_core = true;
                        msg->id = r->getCoreId();
                        msg->type = coyote::MessageType::NOC_MSG;
                        tile->out_port_arbiter_.send(msg, 0);
                break;
            }
            case ScratchpadRequest::ScratchpadCommand::FREE:
            {
                /*if(!r->isServiced())
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
	                std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                        msg->msg = getScratchpadAckMessage(r);
                        msg->is_core = true;
                        msg->id = r->getCoreId();
                        msg->type = coyote::MessageType::NOC_MSG;
                        tile->out_port_arbiter_.send(msg, 0);
                    }
                }
                else
                {
                    pending_scratchpad_management_ops[r]=pending_scratchpad_management_ops[r]-1;

                    if(pending_scratchpad_management_ops[r]==0)
                    {
                        //SEND ACK TO MCPU
	                std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                        msg->msg = getScratchpadAckMessage(r);
                        msg->is_core = true;
                        msg->id = r->getCoreId();
                        msg->type = coyote::MessageType::NOC_MSG;
                        tile->out_port_arbiter_.send(msg, 0);
                        pending_scratchpad_management_ops.erase(r);
                    }
                }*/
                std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                        msg->msg = getScratchpadAckMessage(r);
                        msg->is_core = true;
                        msg->id = r->getCoreId();
                        msg->type = coyote::MessageType::NOC_MSG;
                        tile->out_port_arbiter_.send(msg, 0);
                break;
            }
            case ScratchpadRequest::ScratchpadCommand::READ:
            {
                if(!r->isServiced())
                {
                    int lines_to_read=ceil(1.0*r->getSize()/line_size);

                    //Send as many reads to the bank as lines are necessary to fulfill the request
                    for(int i=0;i<lines_to_read;i++)
                    {
                        uint16_t b=calculateBank(r);
                        r->setCacheBank(b);
                        tile->issueLocalRequest_(r, 1);
                    }
                    pending_scratchpad_management_ops[r]=lines_to_read;
                }
                else
                {
                    pending_scratchpad_management_ops[r]=pending_scratchpad_management_ops[r]-1;

                    //If all the pending ops are done, mark as finished
                    if(pending_scratchpad_management_ops[r]==0)
                    {
                        pending_scratchpad_management_ops.erase(r);

                        // We need to generate a new identical pointer to be sent and then set is as ready. If we reused the old pointer and set it to ready, then 
                        // the MCPU might get visibility on ready==true from an earlier in flight packet that holds the same pointer
                        uint32_t old_id = r->getID();
                        r=std::make_shared<ScratchpadRequest>(r->getAddress(), ScratchpadRequest::ScratchpadCommand::READ, r->getPC(), r->getTimestamp(), r->getCoreId(), r->getSourceTile(), r->getDestinationRegId());

                        r->setOperandReady(true);
                        r->setID(old_id);
                    }
                    //Send ACK to MCPU
	            std::shared_ptr<ArbiterMessage> msg = std::make_shared<ArbiterMessage>();
                    msg->msg = getScratchpadAckMessage(r);
                    msg->is_core = true;
                    msg->id = r->getCoreId();
                    msg->type = coyote::MessageType::NOC_MSG;
                    tile->out_port_arbiter_.send(msg, 0);
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
     
    void AccessDirector::setMemoryInfo(uint64_t size_kbs, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile, uint16_t num_tiles, uint64_t num_mcs, uint64_t memory_controller_shift, uint64_t memory_controller_mask, uint16_t cores)
    {
        this->line_size=line_size;

        block_offset_bits=(uint8_t)(ceil(log2(line_size)));
        tile_bits=(uint8_t)ceil(log2(num_tiles));
        bank_bits=(uint8_t)ceil(log2(banks_per_tile));
        vreg_bits=(uint8_t)ceil(log2(num_vregs_per_core));
        core_bits=(uint8_t)ceil(log2(tile->num_cores_));
        core_to_bank_interleaving_bits=(uint8_t)log2(banks_per_tile/tile->num_cores_);

        uint64_t s=totalSize(size_kbs, num_tiles);
        uint64_t num_sets=s/(assoc*line_size);
        set_bits=(uint8_t)ceil(log2(num_sets));
        tag_bits=64-(set_bits+block_offset_bits);
        num_ways=assoc;
        way_size=(size_kbs/num_ways)*1024;

        mc_shift=memory_controller_shift;
        mc_mask=memory_controller_mask;
        cores_per_tile=cores;
    }

    uint16_t AccessDirector::getCoresPerTile()
    {
        return cores_per_tile;
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
        return std::make_shared<NoCMessage>(req, NoCMessageType::SCRATCHPAD_ACK, 15, tile->id_, req->getSourceTile());
    }

    uint16_t AccessDirector::calculateBank(std::shared_ptr<coyote::ScratchpadRequest> r)
    {
        uint16_t destination=0;
        if(bank_bits>0) //If more than one bank
        {
            switch(scratchpad_data_mapping_policy_)
            {
                case VRegMappingPolicy::CORE_TO_BANK:
                    //Convert to [core_id_bits,vreg_id_bits] format and get the bank_bits most significant bits
                    destination=(((r->getCoreId() % getCoresPerTile()) << vreg_bits) | r->getDestinationRegId()) >> (vreg_bits+core_bits-bank_bits);
                    break;
                case VRegMappingPolicy::VREG_INTERLEAVING:
                    break;
            }
        }
        //printf("Destination is %d\n", destination);
        return destination;
    }

}
