

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
        num_tiles_(p->num_tiles),
        num_memory_controllers_(p->num_memory_controllers),
        latency_(p->latency),
        in_ports_tiles_(num_tiles_),
        out_ports_tiles_(num_tiles_),
        in_ports_memory_controllers_(num_memory_controllers_),
        out_ports_memory_controllers_(num_memory_controllers_)
    {
            for(uint16_t i=0; i<num_tiles_; i++)
            {
                std::string out_name=std::string("out_tile") + sparta::utils::uint32_to_str(i);
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, out_name);
                out_ports_tiles_[i]=std::move(out);
                
                std::string in_name=std::string("in_tile") + sparta::utils::uint32_to_str(i); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>> in=std::make_unique<sparta::DataInPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, in_name);
                in_ports_tiles_[i]=std::move(in);
                in_ports_tiles_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(NoC, handleMessageFromTile_, std::shared_ptr<NoCMessage>));
            }
            
            for(uint16_t i=0; i<num_memory_controllers_; i++)
            {
                std::string out_name=std::string("out_memory_controller") + sparta::utils::uint32_to_str(i);
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, out_name);
                out_ports_memory_controllers_[i]=std::move(out);

                std::string in_name=std::string("in_memory_controller") + sparta::utils::uint32_to_str(i); 
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>> in=std::make_unique<sparta::DataInPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, in_name);
                in_ports_memory_controllers_[i]=std::move(in);
                in_ports_memory_controllers_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(NoC, issueAck_, std::shared_ptr<NoCMessage>));
            }
    }

    void NoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mes)
    {
        switch(mes->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
                //std::cout << "Handling remote request for core " << mes->getRequest()->getCoreId() << " for @ " << mes->getRequest()->getAddress() << " to tile " << mes->getRequest()->getHomeTile() << "\n";
                out_ports_tiles_[mes->getRequest()->getHomeTile()]->send(mes, latency_);
                break;

            case NoCMessageType::MEMORY_REQUEST:
                if(trace_)
                {
                    logger_.logMemoryControllerRequest(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getMemoryController(num_memory_controllers_));
                }
                out_ports_memory_controllers_[mes->getMemoryController(num_memory_controllers_)]->send(mes, latency_);
                break;

            case NoCMessageType::REMOTE_L2_ACK:
                out_ports_tiles_[mes->getRequest()->getSourceTile()]->send(mes, latency_);
                break;

            case NoCMessageType::MEMORY_ACK:
                std::cout << "Memory acks should not use tile ports!!!\n";
                break;

            default:
                std::cout << "Unsupported message received from a Tile!!!\n";
        }
    }

    void NoC::issueAck_(const std::shared_ptr<NoCMessage> & mes)
    {
        if(mes->getType()==NoCMessageType::MEMORY_ACK)
        {
            //std::cout << "Sending memory ack from NoC to request from core " << mes->getRequest()->getCoreId() << " for address " << mes->getRequest()->getAddress() << "\n";
            out_ports_tiles_[mes->getRequest()->getHomeTile()]->send(mes, latency_);
        }
        else
        {
            std::cout << "Unsupported message received from a Memory Controller!!!\n";
        }
    }
        
}
