#include "Arbiter.hpp"
#include <sparta/app/Simulation.hpp>
#include <sparta/app/SimulationConfiguration.hpp>

namespace spike_model
{
    const char Arbiter::name[] = "arbiter";

    Arbiter::Arbiter(sparta::TreeNode* node, const ArbiterParameterSet *p) : sparta::Unit(node)
    {
        current_cycle = 0;
        //TODO: This should be improved and this only represent the NoC networks, in the future crossbar will be more outputs
        // Also, using the UnboundParameterTree, the parameter needs to be in the config file and the parameter has a default value that 
        // should be used.
        //std::string noc_networks = node->getRoot()->getAs<sparta::RootTreeNode>()->getSimulator()->getSimulationConfiguration()
          //                                      ->getUnboundParameterTree().tryGet("top.cpu.noc.params.noc_networks")->getAs<std::string>();
        //num_outputs_ = std::count(noc_networks.begin(), noc_networks.end(), ',') + 1;
        num_outputs_ = 3;
        node_ = node;

        std::string in_name=std::string("in_tile");
        std::unique_ptr<sparta::DataInPort<std::shared_ptr<ArbiterMessage>>> in_=std::make_unique<sparta::DataInPort<std::shared_ptr<ArbiterMessage>>> (&unit_port_set_, in_name);
        in_ports_tile_=std::move(in_);
        in_ports_tile_->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(Arbiter, submit, std::shared_ptr<ArbiterMessage>));
    }

    void Arbiter::submit(const std::shared_ptr<ArbiterMessage> & msg)
    {
        int noc_network = msg->nocmsg->getNoCNetwork();
        int input_unit = getInputIndex(msg->is_core, msg->id);

        addNoCMsg(msg->nocmsg, noc_network, input_unit);
    }

    size_t Arbiter::queueSize(int network_type, int input_unit)
    {
        return pending_noc_msgs[network_type][input_unit].size();
    }

    void Arbiter::addNoCMsg(std::shared_ptr<NoCMessage> mes, int network_type, int input_unit)
    {
        pending_noc_msgs[network_type][input_unit].push(mes);
    }

    std::shared_ptr<NoCMessage> Arbiter::getNoCMsg(int network_type, int input_unit)
    {
        return pending_noc_msgs[network_type][input_unit].front();
    }

    std::shared_ptr<NoCMessage> Arbiter::popNoCMsg(int network_type, int input_unit)
    {
        std::shared_ptr<NoCMessage> msg = pending_noc_msgs[network_type][input_unit].front();
        pending_noc_msgs[network_type][input_unit].pop();
        return msg;
    }

    bool Arbiter::hasNoCMsgInNetwork()
    {
        for(int i = 0; i < (int)num_outputs_; i++)
        {
            for(size_t j = 0; j < pending_noc_msgs[i].size(); j++)
            {
                if(!pending_noc_msgs[i][j].empty())
                    return true;
            }
        }
        return false;
    }

    bool Arbiter::hasNoCMsg(int network_type, int input_unit)
    {
        return !pending_noc_msgs[network_type][input_unit].empty();
    }

    /*
     First num_cores slots are reserved each for a core in the VAS tile
     Second num_l2_banks slots are reserved one each for a L2 bank in a VAS tile
     This function is not used as of now
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

    bool Arbiter::isCore(int j)
    {
        if(j < cores_per_tile_)
            return true;
        return false;
    }

    bool Arbiter::isInputBelowWatermark(int j)
    {
        for(int i = 0; i < (int)num_outputs_;i++)
            if(queueSize(i,j) > threshold)
                return false;
        return true;
    }

    void Arbiter::addBank(CacheBank* bank)
    {
        l2_banks.push_back(bank);
    }

    CacheBank* Arbiter::getBank(int index)
    {
        return l2_banks[index];
    }

    void Arbiter::submitToNoC(uint64_t current_cycle)
    {
        current_cycle *= 3;
        for(int i = 0; i < (int)num_outputs_;i++)
        {
            int j = (rr_cntr[i] + 1) % num_inputs_;
            int cntr = 0;
            while(cntr != num_inputs_)
            {
                if(hasNoCMsg(i, j))
                {
                    //schedule_next = true;
                    std::shared_ptr<NoCMessage> msg = getNoCMsg(i, j);
                    if(noc->checkSpaceForPacket(true, msg))
                    {
                        noc->handleMessageFromTile_(popNoCMsg(i,j));
                        rr_cntr[i] = j;
                        current_cycle = getClock()->currentCycle();
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

    void Arbiter::setRequestManager(std::shared_ptr<EventManager> r)
    {
        request_manager_=r;
    }

    void Arbiter::setNoC(NoC *noc)
    {
        this->noc = noc;
    }

    void Arbiter::setNumInputs(uint16_t cores_per_tile, uint16_t l2_banks_per_tile, uint16_t tile_id)
    {
        rr_cntr.resize((int)num_outputs_, -1);
        num_inputs_ = cores_per_tile + l2_banks_per_tile;
        pending_noc_msgs.resize((int)num_outputs_, std::vector<std::queue<std::shared_ptr<NoCMessage>>>(num_inputs_));
        tile_id_ = tile_id;
        cores_per_tile_ = cores_per_tile;
    }
}
