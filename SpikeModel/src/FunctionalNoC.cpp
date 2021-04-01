
#include "FunctionalNoC.hpp"
#include "sparta/utils/SpartaAssert.hpp"

namespace spike_model
{
    FunctionalNoC::FunctionalNoC(sparta::TreeNode *node, const FunctionalNoCParameterSet *params) :
        NoC(node, params),
        packet_latency_(params->packet_latency)
    {
        sparta_assert(noc_model_ == "functional");
    }

    void FunctionalNoC::handleMessageFromTile_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the statistics
        NoC::handleMessageFromTile_(mess);
        switch(mess->getType())
        {
            case NoCMessageType::REMOTE_L2_REQUEST:
                out_ports_tiles_[mess->getDestPort()]->send(mess, packet_latency_);
                break;

            case NoCMessageType::MEMORY_REQUEST:
                /*if(trace_)
                {
                    logger_.logMemoryControllerRequest(getClock()->currentCycle(), mess->getRequest()->getCoreId(), mess->getRequest()->getPC(), mess->getRequest()->getMemoryController(), mess->getRequest()->getAddress());
                }*/
                out_ports_memory_cpus_[mess->getDestPort()]->send(mess, packet_latency_);
                break;

            case NoCMessageType::REMOTE_L2_ACK:
                out_ports_tiles_[mess->getDestPort()]->send(mess, packet_latency_);
                break;

            case NoCMessageType::MCPU_REQUEST:
                //std::cout << "NOC forwarding the message to MCPU" << std::endl;
                out_ports_memory_cpus_[mess->getDestPort()]->send(mess, packet_latency_);
                break;

            default:
                sparta_assert(false);
        }
    }

    void FunctionalNoC::handleMessageFromMemoryCPU_(const std::shared_ptr<NoCMessage> & mess)
    {
        // Call to parent class to fill the statistics
        NoC::handleMessageFromMemoryCPU_(mess);
        switch(mess->getType())
        {
            case NoCMessageType::MEMORY_ACK:
                /*if(trace_)
                {
                    logger_.logMemoryControllerAck(getClock()->currentCycle(), mess->getRequest()->getCoreId(), mess->getRequest()->getPC(), mess->getRequest()->getHomeTile(), mess->getRequest()->getAddress());
                }*/
                out_ports_tiles_[mess->getDestPort()]->send(mess, packet_latency_);
                break;

            case NoCMessageType::MCPU_REQUEST:
                //std::cout << "NOC forwarding the message to Tile" << std::endl;
                out_ports_tiles_[mess->getDestPort()]->send(mess, packet_latency_);
                break;

            default:
                sparta_assert(false);
        }
    }

} // spike_model
