
#include "sparta/utils/SpartaAssert.hpp"
#include "Tile.hpp"
#include <chrono>

namespace spike_model
{
    const char Tile::name[] = "tile";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    Tile::Tile(sparta::TreeNode *node, const TileParameterSet *p) :
        sparta::Unit(node),
        num_l2_banks_(p->num_l2_banks),
        latency_(p->latency),
        in_ports_l2_acks_(num_l2_banks_),
        in_ports_l2_reqs_(num_l2_banks_),
        out_ports_l2_acks_(num_l2_banks_),
        out_ports_l2_reqs_(num_l2_banks_)
    {
            for(uint16_t i=0; i<num_l2_banks_; i++)
            {
                std::string out_name=std::string("out_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_req");
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, out_name);
                out_ports_l2_reqs_[i]=std::move(out);
                
                out_name=std::string("out_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_ack");
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> out_ack=std::make_unique<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, out_name);
                out_ports_l2_acks_[i]=std::move(out_ack);

                std::string in_name=std::string("in_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_ack"); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<CacheRequest>>> in_ack=std::make_unique<sparta::DataInPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, in_name);
                in_ports_l2_acks_[i]=std::move(in_ack);
                in_ports_l2_acks_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, notifyAck_, std::shared_ptr<CacheRequest>));
                
                in_name=std::string("in_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_req"); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<CacheRequest>>> in_req=std::make_unique<sparta::DataInPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, in_name);
                in_ports_l2_reqs_[i]=std::move(in_req);
                in_ports_l2_reqs_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, issueMemoryControllerRequest_, std::shared_ptr<CacheRequest>));
            }

            in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, handleNoCMessage_, std::shared_ptr<NoCMessage>));
    }

    void Tile::issueMemoryControllerRequest_(const std::shared_ptr<CacheRequest> & req)
    {
            //std::cout << "Issuing memory controller request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << "\n";
            std::shared_ptr<NoCMessage> mes=request_manager_->getMemoryRequestMessage(req);
            out_port_noc_.send(mes);  
    }

    void Tile::issueRemoteRequest_(const std::shared_ptr<CacheRequest> & req, uint64_t lapse)
    {
            out_port_noc_.send(request_manager_->getRemoteL2RequestMessage(req), lapse);  
    }

    void Tile::issueLocalRequest_(const std::shared_ptr<CacheRequest> & req, uint64_t lapse)
    {
        //uint16_t bank=req->calculateHome();
        uint16_t bank=req->getCacheBank(); //TODO: THIS MUST BE FIXED!!
        out_ports_l2_reqs_[bank]->send(req, lapse+latency_);
        //out_ports_[req->getCoreId()]->send(req, 0);
    }

    void Tile::issueBankAck_(const std::shared_ptr<CacheRequest> & req)
    {
        uint16_t bank=req->getCacheBank();
    //    std::cout << "Issuing ack to bank " << (uint16_t)bank << " for request replied from core " << req->getCoreId() << " for address " << req->getAddress() << "\n";
        out_ports_l2_acks_[bank]->send(req);
    }

    void Tile::putRequest_(const std::shared_ptr<CacheRequest> & req)
    {
        handle(req);
    }

    void Tile::notifyAck_(const std::shared_ptr<CacheRequest> & req)
    {
        handle(req);
    }

    void Tile::handleNoCMessage_(const std::shared_ptr<NoCMessage> & mes)
    {
        switch(mes->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
                //std::cout << "Issuing local l2 request for remote request for core " << mes->getRequest()->getCoreId() << " for @ " << mes->getRequest()->getAddress() << " from tile " << id_ << "\n";
                if(trace_)
                {
                    logger_.logSurrogateBankRequest(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getRequest()->getPC(), mes->getRequest()->getHomeTile(), mes->getRequest()->getAddress());
                }
                Tile::issueLocalRequest_(mes->getRequest(), 0);
                break;

            case NoCMessageType::MEMORY_REQUEST:
                //SHOULD DO SOMETHING HERE??
                break;

            case NoCMessageType::REMOTE_L2_ACK:
                //std::cout << "Handling remote ack\n";
                if(trace_)
                {
                    logger_.logTileRecAckForwarded(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getRequest()->getPC(), mes->getRequest()->getAddress());
                    logger_.logMissServiced(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getRequest()->getPC(), mes->getRequest()->getAddress());
                }
                request_manager_->notifyAck(mes->getRequest());
                break;

            case NoCMessageType::MEMORY_ACK:
                if(trace_)
                {
                    logger_.logTileRecAck(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getRequest()->getPC(), mes->getRequest()->getAddress());
                }
                issueBankAck_(mes->getRequest()); 
                break;

            default:
                std::cout << "Unsupported message received from the NoC!!!\n";
        }
    }    
            
    void Tile::setL2BankInfo(uint64_t size, uint64_t assoc, uint64_t line_size)
    {
        l2_bank_size_kbs=size;
        l2_assoc=assoc;
        l2_line_size=line_size;

        block_offset_bits=(uint8_t)ceil(log2(l2_line_size));
        bank_bits=(uint8_t)ceil(log2(in_ports_l2_reqs_.size()));

        uint64_t total_l2_size=l2_bank_size_kbs*in_ports_l2_reqs_.size()*1024;
        uint64_t num_sets=total_l2_size/(l2_assoc*l2_line_size);
        set_bits=(uint8_t)ceil(log2(num_sets));
        tag_bits=64-(set_bits+block_offset_bits);
        std::cout << "There are " << unsigned(bank_bits) << " bits for banks and " << unsigned(set_bits) << " bits for sets\n";
    }

    void Tile::setRequestManager(std::shared_ptr<RequestManagerIF> r)
    {
        request_manager_=r;
    }

    void Tile::setId(uint16_t id)
    {
        id_=id;
    }
    
    void Tile::handle(std::shared_ptr<spike_model::CacheRequest> r)
    {
        if(!r->isServiced())
        {
            if(r->getHomeTile()==id_)
            {
                //std::cout << "Issuing local l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse  << "\n";
                if(trace_)
                {
                    logger_.logLocalBankRequest(r->getTimestamp(), r->getCoreId(), r->getPC(), r->getCacheBank(), r->getAddress());
                }
                issueLocalRequest_(r, r->getTimestamp()-getClock()->currentCycle()); //MAYBE +1?
            }
            else
            {
                if(trace_)
                {
                    logger_.logRemoteBankRequest(r->getTimestamp(), r->getCoreId(), r->getPC(), r->getHomeTile(), r->getAddress());
                }
                //std::cout << "Issuing remote l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse << "\n";
                issueRemoteRequest_(r, r->getTimestamp()-getClock()->currentCycle());
            }
        }
        else
        {
            if(r->getType()==CacheRequest::AccessType::STORE || r->getType()==CacheRequest::AccessType::WRITEBACK)
            {
                logger_.logMissServiced(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress());
                request_manager_->notifyAck(r);
            }
            else
            {
                //The ack is for this tile
                if(r->getSourceTile()==id_)
                {
                    //std::cout << "Notifying to manager\n";
                    if(trace_)
                    {
                        logger_.logMissServiced(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getAddress());
                    }
                    request_manager_->notifyAck(r);
                }
                else
                {
                    //std::cout << "Sending ack to remote\n";
                    if(trace_)
                    {
                        logger_.logTileSendAck(getClock()->currentCycle(), r->getCoreId(), r->getPC(), r->getSourceTile(), r->getAddress());
                    }
                    out_port_noc_.send(request_manager_->getDataForwardMessage(r), l2_line_size); 
                }
            }
        }
    }
    
}
