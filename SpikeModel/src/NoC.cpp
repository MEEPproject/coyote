

#include "sparta/utils/SpartaAssert.hpp"
#include "NoC.hpp"
#include <chrono>

namespace spike_model
{
    const char NoC::name[] = "noc";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    NoC::NoC(sparta::TreeNode *node, const NoCParameterSet *p) :
        sparta::Unit(node),
        num_cores_(p->num_cores),
        num_l2_banks_(p->num_l2_banks),
        in_ports_l2_(num_l2_banks_),
        out_ports_l2_(num_l2_banks_),
        in_ports_cores_(num_cores_),
        out_ports_cores_(num_cores_),
        cores_(num_cores_)
        
    {
            for(uint16_t i=0; i<num_l2_banks_; i++)
            {
                std::string out_name=std::string("out_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_req");
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<L2Request>>> (&unit_port_set_, out_name);
                out_ports_l2_[i]=std::move(out);

                std::string in_name=std::string("in_l2_bank") + sparta::utils::uint32_to_str(i) + std::string("_ack"); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>> in=std::make_unique<sparta::DataInPort<std::shared_ptr<L2Request>>> (&unit_port_set_, in_name);
                in_ports_l2_[i]=std::move(in);
                in_ports_l2_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(NoC, issueAck_, std::shared_ptr<L2Request>));
            }

            for(uint16_t i=0; i<num_cores_; i++)  
            { 
                std::string out_name=std::string("out_core") + sparta::utils::uint32_to_str(i);
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>> out_core=std::make_unique<sparta::DataOutPort<std::shared_ptr<L2Request>>> (&unit_port_set_, out_name);
                out_ports_cores_[i]=std::move(out_core);

                std::string in_name=std::string("in_core") + sparta::utils::uint32_to_str(i); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>> in_core=std::make_unique<sparta::DataInPort<std::shared_ptr<L2Request>>> (&unit_port_set_, in_name);
                in_ports_cores_[i]=std::move(in_core);
                in_ports_cores_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(NoC, send_, std::shared_ptr<L2Request>));
            }

            if(p->data_mapping_policy=="set_interleaving")
            {
                data_mapping_policy_=MappingPolicy::SET_INTERLEAVING; 
            }
            else if(p->data_mapping_policy=="page_to_bank")
            {
                data_mapping_policy_=MappingPolicy::PAGE_TO_BANK; 
            }
            else
            {
                throw sparta::SpartaException("Invalid data mapping policy ") << p->data_mapping_policy << "'";
            }
    }

    void NoC::send_(const std::shared_ptr<L2Request> & req)
    {
        uint8_t bank=getDestination(req);
        out_ports_l2_[bank]->send(req, 1);
        //out_ports_[req->getCoreId()]->send(req, 0); 
    }

    void NoC::issueAck_(const std::shared_ptr<L2Request> & req)
    {
        out_ports_cores_[req->getCoreId()]->send(req);
    }
        
    uint16_t NoC::getDestination(std::shared_ptr<L2Request> req)
    {
        uint16_t destination=0;
        
        if(bank_bits>0) //If more than one bank
        {
            uint8_t left=tag_bits;
            uint8_t right=block_offset_bits;
            switch(data_mapping_policy_)
            {
                case MappingPolicy::SET_INTERLEAVING:
                    left+=set_bits-bank_bits;
                    break;
                case MappingPolicy::PAGE_TO_BANK:
                    right+=set_bits-bank_bits;
                    break;
            }
            destination=(req->getAddress() << left) >> (left+right);
        }
        return destination;
    }
}
