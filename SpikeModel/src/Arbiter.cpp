#include "Arbiter.hpp"

namespace spike_model
{
    const char Arbiter::name[] = "arbiter";

    Arbiter::Arbiter(sparta::TreeNode* node, const ArbiterParameterSet *p) : sparta::Unit(node)
    {
    }

    void Arbiter::submit(std::shared_ptr<NoCMessage> msg, bool is_core, int id)
    {
        int noc_network = msg->getNoCNetwork();
        int input_unit = getInputIndex(is_core, id);

        if(hasNoCMsgInNetwork(noc_network))
        {
	    addNoCMsg(msg, noc_network, input_unit);
        }
        else
        {
            addNoCMsg(msg, noc_network, input_unit);
            pending_arbiter_event_.schedule(sparta::Clock::Cycle(0));
        }
    }

    void Arbiter::addNoCMsg(std::shared_ptr<NoCMessage> mes, int network_type, int input_unit)
    {
        pending_noc_msgs[network_type][input_unit].push(mes);
    }

    std::shared_ptr<NoCMessage> Arbiter::getNoCMsg(int network_type, int input_unit)
    {
        std::shared_ptr<NoCMessage> msg = pending_noc_msgs[network_type][input_unit].front();
        pending_noc_msgs[network_type][input_unit].pop();
        return msg;
    }

    bool Arbiter::hasNoCMsgInNetwork(int network_type)
    {
        for(size_t i = 0; i < pending_noc_msgs[network_type].size();i++)
        {
            if(!pending_noc_msgs[network_type][i].empty())
                return true;
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

    void Arbiter::submitToNoc()
    {
        bool schedule_next = false;
        for(int i = 0; i < (int)NoC::Networks::count;i++)
        {
            int j = (rr_cntr[i] + 1) % num_inputs_;
            int cntr = 0;
            while(cntr != num_inputs_)
            {
                if(hasNoCMsg(i, j))
                {
                    out_port_noc_.send(getNoCMsg(i, j));
                    schedule_next = true;
                    rr_cntr[i] = j;
                    break;
                }
                else
                {
                   j = (j + 1) % num_inputs_;
                   cntr++;
                }
            }
        }
        if(schedule_next)
            pending_arbiter_event_.schedule(sparta::Clock::Cycle(1));
    }

    void Arbiter::setNumInputs(uint16_t cores_per_tile, uint16_t l2_banks_per_tile)
    {
        rr_cntr.resize((int)NoC::Networks::count, 0);
        cores_per_tile_ = cores_per_tile;
        num_inputs_ = cores_per_tile + l2_banks_per_tile;
        pending_noc_msgs.resize((int)NoC::Networks::count, std::vector<std::queue<std::shared_ptr<NoCMessage>>>(num_inputs_));
    }
}
