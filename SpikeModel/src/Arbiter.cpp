#include "Arbiter.hpp"
#include <sparta/app/Simulation.hpp>
#include <sparta/app/SimulationConfiguration.hpp>

namespace spike_model
{
    const char Arbiter::name[] = "arbiter";

    Arbiter::Arbiter(sparta::TreeNode* node, const ArbiterParameterSet *p) : sparta::Unit(node), q_sz(p->q_sz)
    {
        std::cout << "Q SIZE " << q_sz << std::endl;
        //TODO: This should be improved and this only represent the NoC networks, in the future crossbar will be more outputs
        // Also, using the UnboundParameterTree, the parameter needs to be in the config file and the parameter has a default value that 
        // should be used.
        std::string noc_networks = node->getRoot()->getAs<sparta::RootTreeNode>()->getSimulator()->getSimulationConfiguration()
                                                ->getUnboundParameterTree().tryGet("top.cpu.noc.params.noc_networks")->getAs<std::string>();
        num_outputs_ = std::count(noc_networks.begin(), noc_networks.end(), ',') + 1;

        std::string in_name=std::string("in_tile");
        std::unique_ptr<sparta::DataInPort<std::shared_ptr<ArbiterMessage>>> in_=std::make_unique<sparta::DataInPort<std::shared_ptr<ArbiterMessage>>> (&unit_port_set_, in_name);
        in_ports_tile_=std::move(in_);
        in_ports_tile_->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Arbiter, submit, std::shared_ptr<ArbiterMessage>));
    }

    void Arbiter::submit(const std::shared_ptr<ArbiterMessage> & msg)
    {
        if(msg->type == spike_model::MessageType::CACHE_REQUEST)
        {
            std::shared_ptr<CacheRequest> req = std::static_pointer_cast<CacheRequest>(msg->msg);
            uint16_t bank = req->getCacheBank();
            uint16_t core = req->getCoreId();
            addCacheRequest(req, bank, getInputIndex(true, core));
        }
        else
        {
            std::shared_ptr<NoCMessage> nocmsg = std::static_pointer_cast<NoCMessage>(msg->msg);
            int noc_network = nocmsg->getNoCNetwork();
            int input_unit = getInputIndex(msg->is_core, msg->id);
            addNoCMsg(nocmsg, noc_network, input_unit);
        }
    }

    void Arbiter::addNoCMsg(std::shared_ptr<NoCMessage> mes, int network_type, int input_unit)
    {
        pending_noc_msgs_[network_type][input_unit].push(mes);
    }

    std::shared_ptr<NoCMessage> Arbiter::getNoCMsg(int network_type, int input_unit)
    {
        return pending_noc_msgs_[network_type][input_unit].front();
    }

    std::shared_ptr<NoCMessage> Arbiter::popNoCMsg(int network_type, int input_unit)
    {
        std::shared_ptr<NoCMessage> msg = pending_noc_msgs_[network_type][input_unit].front();
        pending_noc_msgs_[network_type][input_unit].pop();
        return msg;
    }

    bool Arbiter::hasNoCMsgInNetwork()
    {
        for(int i = 0; i < (int)num_outputs_; i++)
        {
            for(size_t j = 0; j < pending_noc_msgs_[i].size(); j++)
            {
                if(!pending_noc_msgs_[i][j].empty())
                    return true;
            }
        }
        return false;
    }

    bool Arbiter::hasNoCMsg(int network_type, int input_unit)
    {
        return !pending_noc_msgs_[network_type][input_unit].empty();
    }

    void Arbiter::addCacheRequest(std::shared_ptr<CacheRequest> req, uint16_t bank, int core)
    {
        pending_l2_msgs_[bank][core].push(req);
    }

    std::shared_ptr<CacheRequest> Arbiter::popCacheRequest(uint16_t bank, int core)
    {
        std::shared_ptr<CacheRequest> msg = pending_l2_msgs_[bank][core].front();
        pending_l2_msgs_[bank][core].pop();
        return msg;
    }

    bool Arbiter::hasCacheRequestInNetwork()
    {
        for(int i = 0; i < num_l2_banks_; i++)
        {
            for(size_t j = 0; j < pending_l2_msgs_[i].size(); j++)
            {
                if(!pending_l2_msgs_[i][j].empty())
                    return true;
            }
        }
        return false;
    }

    bool Arbiter::hasCacheRequest(uint16_t bank, int core)
    {
        return !pending_l2_msgs_[bank][core].empty();
    }

    /*
     First num_cores slots are reserved each for a core in the VAS tile
     Second num_l2_banks slots are reserved one each for a L2 bank in a VAS tile
    */
    int Arbiter::getInputIndex(bool is_core, int id)
    {
        if(is_core)
        {
            return (id % cores_per_tile_);
        }
        else
        {
            return (cores_per_tile_ + id);
        }
    }

    bool Arbiter::hasNoCQueueFreeSlot(uint16_t j)
    {
        for(int i = 0; i < (int)num_outputs_;i++)
            if(NoCQueueSize(i,j) >= q_sz)
                return false;
        return true;
    }

    bool Arbiter::hasL1L2QueueFreeSlot(uint16_t j)
    {
        for(uint16_t i = 0; i < num_l2_banks_; i++)
            if(L2QueueSize(i,j) >= q_sz)
                return false;
        return true;
    }

    bool Arbiter::hasL2NoCQueueFreeSlot(uint16_t bank_id)
    {
        uint16_t j = getInputIndex(false, bank_id);
        for(int i = 0; i < (int)num_outputs_;i++)
            if(NoCQueueSize(i,j) >= q_sz)
                return false;
        return true;

    }

    bool Arbiter::hasArbiterQueueFreeSlot(uint16_t tile_id, uint16_t core_id)
    {
        bool ret = false;
        uint16_t start_core = tile_id*cores_per_tile_;
        uint16_t end_core = (tile_id+1)*cores_per_tile_;
        if(core_id < start_core || core_id >= end_core)
            return false;
        core_id = getInputIndex(true, core_id);
        ret = hasNoCQueueFreeSlot(core_id);
        ret = ret | hasL1L2QueueFreeSlot(core_id);
        return ret;
    }

    bool Arbiter::isCore(int j)
    {
        if(j < cores_per_tile_)
            return true;
        return false;
    }

    void Arbiter::addBank(CacheBank* bank)
    {
        l2_banks.push_back(bank);
    }

    CacheBank* Arbiter::getBank(int index)
    {
        return l2_banks[index];
    }

    void Arbiter::submitToL2()
    {
        for(int i = 0; i < num_l2_banks_; i++)
        {
            int j = (rr_cntr_cache_req_[i] + 1) % cores_per_tile_;
            int cntr = 0;
            while(cntr != cores_per_tile_)
            {
                if(hasCacheRequest(i, j))
                {
                    l2_banks[i]->getAccess_(popCacheRequest(i,j));
                    rr_cntr_cache_req_[i] = j;
                    break;
                }
                else
                {
                    j = (j + 1) % cores_per_tile_;
                    cntr++;
                }
            }
        }
    }

    size_t Arbiter::NoCQueueSize(int network_type, int input_unit)
    {
        return pending_noc_msgs_[network_type][input_unit].size();
    }

    size_t Arbiter::L2QueueSize(uint16_t bank, uint16_t core)
    {
        return pending_l2_msgs_[bank][core].size();
    }

    void Arbiter::submitToNoC()
    {
        for(int i = 0; i < (int)num_outputs_;i++)
        {
            int j = (rr_cntr_noc_[i] + 1) % num_inputs_;
            int cntr = 0;
            while(cntr != num_inputs_)
            {
                if(hasNoCMsg(i, j))
                {
                    std::shared_ptr<NoCMessage> msg = getNoCMsg(i, j);
                    if(noc->checkSpaceForPacket(true, msg))
                    {
                        noc->handleMessageFromTile_(popNoCMsg(i,j));
                        rr_cntr_noc_[i] = j;
                        break;
                    }
                    else
                    {
                        //Other smaller packet could go through
                        j = (j + 1) % num_inputs_;
                        cntr++;
                    }
                }
                else
                {
                    j = (j + 1) % num_inputs_;
                    cntr++;
                }
            }
        }
    }

    void Arbiter::setNoC(NoC *noc)
    {
        this->noc = noc;
    }

    void Arbiter::setNumInputs(uint16_t cores_per_tile, uint16_t l2_banks_per_tile, uint16_t tile_id)
    {
        rr_cntr_noc_.resize((int)num_outputs_, -1);
        rr_cntr_cache_req_.resize(l2_banks_per_tile, -1);
        num_inputs_ = cores_per_tile + l2_banks_per_tile;
        pending_noc_msgs_.resize((int)num_outputs_, std::vector<std::queue<std::shared_ptr<NoCMessage>>>(num_inputs_));
        pending_l2_msgs_.resize(l2_banks_per_tile, std::vector<std::queue<std::shared_ptr<CacheRequest>>>(cores_per_tile));
        cores_per_tile_ = cores_per_tile;
        num_l2_banks_ = l2_banks_per_tile;
    }
}
