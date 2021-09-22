
#include "sparta/utils/SpartaAssert.hpp"
#include "sparta/app/Simulation.hpp"
#include "Tile.hpp"
#include "PrivateL2Director.hpp"
#include "SharedL2Director.hpp"
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
        l2_sharing_mode_(p->l2_sharing_mode),
        bank_policy_(p->bank_policy),
        scratchpad_policy_(p->scratchpad_policy),
        tile_policy_(p->tile_policy),
        in_ports_l2_acks_(num_l2_banks_),
        in_ports_l2_reqs_(num_l2_banks_),
        out_ports_l2_acks_(num_l2_banks_),
        out_ports_l2_reqs_(num_l2_banks_)
    {
        sparta_assert(l2_sharing_mode_ == "tile_private" || l2_sharing_mode_ == "fully_shared", 
            "The top.cpu.tile*.params.l2_sharing_mode must be tile_private or fully_shared");
        sparta_assert(bank_policy_ == "page_to_bank" || bank_policy_ == "set_interleaving", 
            "The top.cpu.tile*.params.bank_policy must be page_to_bank or set_interleaving");
        sparta_assert(scratchpad_policy_ == "page_to_bank" || scratchpad_policy_ == "set_interleaving",
            "The top.cpu.tile*.params.scratchpad_policy must be page_to_bank or set_interleaving");
        node_ = node;
        arbiter = NULL;
        for(uint16_t i=0; i<num_l2_banks_; i++)
        {
            std::string out_name=std::string("out_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_req");
            std::unique_ptr<sparta::DataOutPort<std::shared_ptr<Request>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<Request>>> (&unit_port_set_, out_name);
            out_ports_l2_reqs_[i]=std::move(out);

            out_name=std::string("out_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_ack");
            std::unique_ptr<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> out_ack=std::make_unique<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, out_name);
            out_ports_l2_acks_[i]=std::move(out_ack);

            std::string in_name=std::string("in_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_ack");
            std::unique_ptr<sparta::DataInPort<std::shared_ptr<Request>>> in_ack=std::make_unique<sparta::DataInPort<std::shared_ptr<Request>>> (&unit_port_set_, in_name);
            in_ports_l2_acks_[i]=std::move(in_ack);
            in_ports_l2_acks_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, notifyAck_, std::shared_ptr<Request>));

            in_name=std::string("in_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_req");
            std::unique_ptr<sparta::DataInPort<std::shared_ptr<CacheRequest>>> in_req=std::make_unique<sparta::DataInPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, in_name);
            in_ports_l2_reqs_[i]=std::move(in_req);
            in_ports_l2_reqs_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, issueMemoryControllerRequest_, std::shared_ptr<CacheRequest>));
        }

        in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Tile, handleNoCMessage_, std::shared_ptr<NoCMessage>));

        spike_model::CacheDataMappingPolicy b_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        if(bank_policy_=="page_to_bank")
        {
            b_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        }
        else if(bank_policy_=="set_interleaving")
        {
            b_pol=spike_model::CacheDataMappingPolicy::SET_INTERLEAVING;
        }
        else
        {
            printf("Unsupported cache data mapping policy\n");
        }

        spike_model::CacheDataMappingPolicy s_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        if(scratchpad_policy_=="page_to_bank")
        {
            s_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
        }
        else if(scratchpad_policy_=="set_interleaving")
        {
            s_pol=spike_model::CacheDataMappingPolicy::SET_INTERLEAVING;
        }
        else
        {
            printf("Unsupported cache data mapping policy\n");
        }

        if(l2_sharing_mode_=="tile_private")
        {
            access_director=new PrivateL2Director(this, b_pol, s_pol);
        }
        else
        {
            spike_model::CacheDataMappingPolicy t_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
            if(tile_policy_=="page_to_bank")
            {
                t_pol=spike_model::CacheDataMappingPolicy::PAGE_TO_BANK;
            }
            else if(tile_policy_=="set_interleaving")
            {
                t_pol=spike_model::CacheDataMappingPolicy::SET_INTERLEAVING;
            }
            else
            {
                printf("Unsupported cache data mapping policy\n");
            }
            access_director=new SharedL2Director(this, b_pol, t_pol, s_pol);
        }
    }

    Arbiter* Tile::getArbiter()
    {
        if(arbiter != NULL)
            return arbiter;
        
        auto core_node = node_->getChild(std::string("arbiter") + sparta::utils::uint32_to_str(id_));
        sparta_assert(core_node != nullptr);

        arbiter=core_node->getResourceAs<spike_model::Arbiter>();
        arbiter->setNumInputs(getCoresPerTile(), num_l2_banks_);
        return arbiter;
    }

    void Tile::issueMemoryControllerRequest_(const std::shared_ptr<CacheRequest> & req)
    {
        //std::cout << "Issuing memory controller request for core " << req->getCoreId() << " for @ " << req->getAddress() << " from tile " << id_ << "\n";
        std::shared_ptr<NoCMessage> mes=access_director->getMemoryRequestMessage(req);
        getArbiter()->submit(mes, false, req->get_l2_bank_id());
    }

    void Tile::issueRemoteRequest_(const std::shared_ptr<CacheRequest> & req, uint64_t lapse)
    {
        std::shared_ptr<NoCMessage> msg = access_director->getRemoteL2RequestMessage(req);
        getArbiter()->submit(msg, true, req->getCoreId());
    }

    void Tile::issueLocalRequest_(const std::shared_ptr<Request> & req, uint64_t lapse)
    {
        //uint16_t bank=req->calculateHome();
        uint16_t bank=req->getCacheBank(); //TODO: THIS MUST BE FIXED!!
        out_ports_l2_reqs_[bank]->send(req, lapse+latency_);
        //out_ports_[req->getCoreId()]->send(req, 0);
    }

    void Tile::issueBankAck_(const std::shared_ptr<CacheRequest> & req)
    {
        uint16_t bank=req->getCacheBank();
        //std::cout << "Issuing ack to bank " << (uint16_t)bank << " for request replied from core " << req->getCoreId() << " for address " << req->getAddress() << std::endl;
        out_ports_l2_acks_[bank]->send(req);
    }

    void Tile::putRequest_(const std::shared_ptr<Event> & req)
    {
        req->handle(this);
    }

    void Tile::notifyAck_(const std::shared_ptr<Request> & req)
    {
        req->handle(this);
    }

    void Tile::handleNoCMessage_(const std::shared_ptr<NoCMessage> & mes)
    {
        switch(mes->getType())
        {
                case NoCMessageType::REMOTE_L2_REQUEST:
                    //TODO: Figure out the correct params
                    //std::cout << "Issuing local l2 request for remote request for core " << mes->getRequest()->getCoreId() << " for @ " << mes->getRequest()->getAddress() << " from tile " << id_ << "\n";
                    /*if(trace_)
                    {
                        logger_.logSurrogateBankRequest(getclock()->currentcycle(), mes->getCoreId(), mes->getPC(), mes->getHomeTile(), mes->getAddress());
                    }*/
                    break;

                case NoCMessageType::MEMORY_REQUEST_LOAD:
                case NoCMessageType::MEMORY_REQUEST_STORE:
                case NoCMessageType::MEMORY_REQUEST_WB:
                    //SHOULD DO SOMETHING HERE??
                    break;

                case NoCMessageType::REMOTE_L2_ACK:
                    //std::cout << "Handling remote ack\n";
                    /*if(trace_)
                    {
                        logger_.logTileRecAckForwarded(getClock()->currentCycle(), mes->getCoreId(), mes->getPC(), mes->getAddress());
                        logger_.logMissServiced(getClock()->currentCycle(), mes->getCoreId(), mes->getPC(), mes->getAddress());
                    }*/
                    break;

                case NoCMessageType::MEMORY_ACK:
                    /*if(trace_)
                    {
                        logger_.logTileRecAck(getClock()->currentCycle(), mes->getCoreId(), mes->getPC(), mes->getAddress());
                    }*/
                    break;

                case NoCMessageType::MCPU_REQUEST:
                    break;
                    
                case NoCMessageType::SCRATCHPAD_COMMAND:
                case NoCMessageType::SCRATCHPAD_DATA_REPLY:
                case NoCMessageType::SCRATCHPAD_ACK:
                    break;

                default:
                    std::cout << "Unsupported message received from the NoC!!!\n";
            }
            mes->getRequest()->handle(this);
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

    void Tile::setRequestManager(std::shared_ptr<EventManager> r)
    {
        request_manager_=r;
    }

    void Tile::setId(uint16_t id)
    {
        id_=id;
    }

    void Tile::handle(std::shared_ptr<spike_model::Request> r)
    {
        access_director->putAccess(r);
    }

    void Tile::handle(std::shared_ptr<spike_model::MCPUSetVVL> r)
    {
        if(!r->isServiced())
        {
            //std::cout << "Issuing MCPU VVL from core " << r->getCoreId() << " and tile " << id_ << std::endl;

            //TODO: The actual MCPU that will handle the request needs to be defined
    	    std::shared_ptr<NoCMessage> msg = std::make_shared<NoCMessage>(r, NoCMessageType::MCPU_REQUEST, 32, id_, corresponding_mcpu);
            getArbiter()->submit(msg, true, r->getCoreId());
        }
        else
        {
            //std::cout << "Acknowledge MCPU request for core " << r->getCoreId() << " from tile " << id_ << std::endl;
            request_manager_->notifyAck(r);
        }
    }

    void Tile::handle(std::shared_ptr<spike_model::MCPUInstruction> r)
    {
        //TODO: The actual MCPU that will handle the request needs to be defined
	std::shared_ptr<NoCMessage> msg = std::make_shared<NoCMessage>(r, NoCMessageType::MCPU_REQUEST, 32, id_, corresponding_mcpu);
        getArbiter()->submit(msg, true, r->getCoreId());
    }

    void Tile::handle(std::shared_ptr<spike_model::InsnLatencyEvent> r)
    {
        insn_latency_event_.preparePayload(r)->schedule(r->getAvailCycle() - getClock()->currentCycle());
    }

    void Tile::setMemoryInfo(uint64_t l2_tile_size, uint64_t assoc,
               uint64_t line_size, uint64_t banks_per_tile, uint16_t num_tiles,
               uint64_t num_mcs, uint64_t mc_shift, uint64_t mc_mask, uint16_t num_cores, uint16_t corr_mcpu)
    {
        setNumTiles(num_tiles);
        setCoresPerTile(num_cores/num_tiles);
        corresponding_mcpu=corr_mcpu;
        access_director->setMemoryInfo(l2_tile_size, assoc, line_size, banks_per_tile, num_tiles, num_mcs, mc_shift, mc_mask);
    }

    void Tile::insnLatencyCallback(const std::shared_ptr<spike_model::InsnLatencyEvent>& r)
    {
        r->setServiced();
        request_manager_->notifyAck(r);
    }
}
