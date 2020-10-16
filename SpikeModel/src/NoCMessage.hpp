
#ifndef __NOC_MESSAGE_HH__
#define __NOC_MESSAGE_HH__

#include <memory>
#include "NoCMessageType.hpp"
#include "L2Request.hpp"
#include "DataMappingPolicy.hpp"

namespace spike_model
{
    class NoCMessage
    {
        public:
            NoCMessage() = delete;
            NoCMessage(NoCMessage const&) = delete;
            NoCMessage& operator=(NoCMessage const&) = delete;
            
            NoCMessage(std::shared_ptr<L2Request> r, NoCMessageType t)
            {
                request_=r;
                message_type_=t;
            }

            NoCMessageType getType(){return message_type_;}
            std::shared_ptr<L2Request> getRequest(){return request_;}

            //This potentially has to be extended to consider different data mapping policies
            uint16_t getMemoryController(uint16_t num_controllers);

        private:
            std::shared_ptr<L2Request> request_;
            NoCMessageType message_type_;
    };
}
#endif
