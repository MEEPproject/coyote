
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
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<L2Request>>> (&unit_port_set_, out_name);
                out_ports_l2_reqs_[i]=std::move(out);
                
                out_name=std::string("out_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_ack");
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>> out_ack=std::make_unique<sparta::DataOutPort<std::shared_ptr<L2Request>>> (&unit_port_set_, out_name);
                out_ports_l2_acks_[i]=std::move(out_ack);

                std::string in_name=std::string("in_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_ack"); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>> in_ack=std::make_unique<sparta::DataInPort<std::shared_ptr<L2Request>>> (&unit_port_set_, in_name);
                in_ports_l2_acks_[i]=std::move(in_ack);
                in_ports_l2_acks_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, notifyAck_, std::shared_ptr<L2Request>));
                
                in_name=std::string("in_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_req"); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>> in_req=std::make_unique<sparta::DataInPort<std::shared_ptr<L2Request>>> (&unit_port_set_, in_name);
                in_ports_l2_reqs_[i]=std::move(in_req);
                in_ports_l2_reqs_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, issueMemoryControllerRequest_, std::shared_ptr<L2Request>));
            }

            in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, handleNoCMessage_, std::shared_ptr<NoCMessage>));
    }

    void Tile::issueMemoryControllerRequest_(const std::shared_ptr<L2Request> & req)
    {
            //std::cout << "Issuing memory controller request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << "\n";
            std::shared_ptr<NoCMessage> mes=std::make_shared<NoCMessage>(req, NoCMessageType::MEMORY_REQUEST);
            out_port_noc_.send(mes);  
    }

    void Tile::issueRemoteL2Request_(const std::shared_ptr<L2Request> & req, uint64_t lapse)
    {
            if(trace_)
            {
                logger_.logRemoteBankRequest(getClock()->currentCycle()+lapse, req->getCoreId(), req->getHomeTile());
            }
            out_port_noc_.send(std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_REQUEST), lapse);  
    }

    void Tile::issueLocalL2Request_(const std::shared_ptr<L2Request> & req, uint64_t lapse)
    {
        //uint16_t bank=req->calculateHome();
        uint16_t bank=req->getBank(); //TODO: THIS MUST BE FIXED!!
        if(trace_)
        {
            logger_.logLocalBankRequest(getClock()->currentCycle()+lapse, req->getCoreId(), bank);
        }
        out_ports_l2_reqs_[bank]->send(req, lapse+latency_);
        //out_ports_[req->getCoreId()]->send(req, 0);
    }

    void Tile::issueBankAck_(const std::shared_ptr<L2Request> & req)
    {
        uint16_t bank=req->getBank();
        //std::cout << "Issuing ack to bank " << (uint16_t)bank << " for request replied from core " << req->getCoreId() << " for address " << req->getAddress() << "\n";
        out_ports_l2_acks_[bank]->send(req);
    }

    void Tile::putRequest_(const std::shared_ptr<L2Request> & req, uint64_t lapse)
    {
        if(req->getHomeTile()==id_)
        {
            //std::cout << "Issuing local l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse  << "\n";
            issueLocalL2Request_(req, lapse);
        }
        else
        {
            //std::cout << "Issuing remote l2 request request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << ". Using lapse " << lapse << "\n";
            issueRemoteL2Request_(req, lapse);
        }
    }

    void Tile::notifyAck_(const std::shared_ptr<L2Request> & req)
    {
        //The ack is for this tile
        if(req->getSourceTile()==id_)
        {
            //std::cout << "Notifying to manager\n";
            request_manager_->notifyAck(req);
        }
        else
        {
            //std::cout << "Sending ack to remote\n";
            out_port_noc_.send(std::make_shared<NoCMessage>(req, NoCMessageType::REMOTE_L2_ACK)); 
        }
    }

    void Tile::handleNoCMessage_(const std::shared_ptr<NoCMessage> & mes)
    {
        switch(mes->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
                //std::cout << "Issuing local l2 request for remote request for core " << mes->getRequest()->getCoreId() << " for @ " << mes->getRequest()->getAddress() << " from tile " << id_ << "\n";
                Tile::issueLocalL2Request_(mes->getRequest(), 0);
                break;

            case NoCMessageType::MEMORY_REQUEST:
                //SHOULD DO SOMETHING HERE??
                break;

            case NoCMessageType::REMOTE_L2_ACK:
                //std::cout << "Handling remote ack\n";
                request_manager_->notifyAck(mes->getRequest());
                break;

            case NoCMessageType::MEMORY_ACK:
                issueBankAck_(mes->getRequest()); 
                break;

            default:
                std::cout << "Unsupported message received from the NoC!!!\n";
        }
    }    
}
