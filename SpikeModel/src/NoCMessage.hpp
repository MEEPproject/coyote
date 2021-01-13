
#ifndef __NOC_MESSAGE_HH__
#define __NOC_MESSAGE_HH__

#include <memory>
#include "NoCMessageType.hpp"
#include "Request.hpp"
#include "RequestManagerIF.hpp"

namespace spike_model
{
    class NoCMessage
    {
        public:
            NoCMessage() = delete;
            NoCMessage(NoCMessage const&) = delete;
            NoCMessage& operator=(NoCMessage const&) = delete;
            
            /*!
             * \brief Constructor for NoCMessage. NoCMessages should only be created for requests with full memory destination info. For this reason they are created in class RequestManagerIF. 
             * Think twice before creating directly elsewhere.
             * \note  r is the associated Request
             *        t is the type of the message
             *        payload_size is the size in bytes of the data to be sent
             */
            NoCMessage(std::shared_ptr<Request> r, NoCMessageType t, uint64_t payload_size);
            

            /*!
             * \brief Returns the type of the message
             */
            NoCMessageType getType(){return message_type_;}
            
            /*!
             * \brief Returns a smart pointer to the Request associated to the message
             */
            std::shared_ptr<Request> getRequest(){return request_;}

            
            /*!
             * \brief Returns the size of the message including its header
             */
            uint64_t getSize();
            

        private:
            std::shared_ptr<Request> request_;
            NoCMessageType message_type_;
            uint64_t size_;

            //The size of message headers in bytes. TODO: MAKE IT A PARAMETER
            static const uint8_t header_size_=1;
            
            
    };
}
#endif
