

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
        num_memory_cpus_(p->num_memory_cpus),
        latency_(p->latency),
        in_ports_tiles_(num_tiles_),
        out_ports_tiles_(num_tiles_),
        in_ports_memory_cpus_(num_memory_cpus_),
        out_ports_memory_cpus_(num_memory_cpus_)
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

            for(uint16_t i=0; i<num_memory_cpus_; i++)
            {
                std::string out_name=std::string("out_memory_cpu") + sparta::utils::uint32_to_str(i);
                std::unique_ptr<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, out_name);
                out_ports_memory_cpus_[i]=std::move(out);

                std::string in_name=std::string("in_memory_cpu") + sparta::utils::uint32_to_str(i);
                std::unique_ptr<sparta::DataInPort<std::shared_ptr<NoCMessage>>> in=std::make_unique<sparta::DataInPort<std::shared_ptr<NoCMessage>>> (&unit_port_set_, in_name);
                in_ports_memory_cpus_[i]=std::move(in);
                in_ports_memory_cpus_[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(NoC, handleMessageFromMemoryCPU_, std::shared_ptr<NoCMessage>));
            }
    }

    void NoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mes)
    {
        switch(mes->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
                out_ports_tiles_[mes->getDestPort()]->send(mes, latency_);
                break;

            case NoCMessageType::MEMORY_REQUEST:
                //TODO: Figure out the correct params
                /*if(trace_)
                {
<<<<<<< HEAD
                    logger_.logMemoryControllerRequest(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getRequest()->getPC(), mes->getRequest()->getMemoryController(), mes->getRequest()->getAddress());
                }*/
                out_ports_memory_cpus_[mes->getDestPort()]->send(mes, latency_);
                break;

            case NoCMessageType::REMOTE_L2_ACK:
                out_ports_tiles_[mes->getDestPort()]->send(mes, latency_);
                break;

            case NoCMessageType::MEMORY_ACK:
                std::cout << "Memory acks should not use tile ports!!!\n";
                break;

            case NoCMessageType::MCPU_REQUEST:
                std::cout << "NOC forwarding the message to MCPU" << std::endl;
                out_ports_memory_cpus_[mes->getDestPort()]->send(mes, 0); // latency = 0 to simulate a separate network for those requests
                break;

            default:
                std::cout << "Unsupported message received from a Tile!!!\n";
        }
    }

    void NoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mes)
    {
        if(mes->getType()==NoCMessageType::MEMORY_ACK)
        {
            //TODO: Figure out the correct params
            /*if(trace_)
            {
<<<<<<< HEAD
                logger_.logMemoryControllerAck(getClock()->currentCycle(), mes->getRequest()->getCoreId(), mes->getRequest()->getPC(), mes->getRequest()->getHomeTile(), mes->getRequest()->getAddress());
            }*/
            out_ports_tiles_[mes->getDestPort()]->send(mes, latency_);
        }
        else if(mes->getType() == NoCMessageType::MCPU_REQUEST)
        {
            std::cout << "NOC forwarding the message to Tile" << std::endl;
            out_ports_tiles_[mes->getDestPort()]->send(mes, 0);
        }
        else
        {
            std::cout << "Unsupported message received from a Memory CPU!!!\n";
        }
    }
}
