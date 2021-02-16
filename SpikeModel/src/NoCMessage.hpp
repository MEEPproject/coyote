
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
        /*!
         * \class spike_model::NoCMessage
         * \brief A representation of a message that wille travel through the NoC.
         *
         * NoCMessages are associated to a particular Request and hold extra information regarding the directionality and size of the message
         */
        public:
            NoCMessage() = delete;
            NoCMessage(NoCMessage const&) = delete;
            NoCMessage& operator=(NoCMessage const&) = delete;
            
            /*!
             * \brief Constructor for NoCMessage. NoCMessages should only be created for requests with full memory destination info. For this reason they are created in class RequestManagerIF. 
             * Think twice before creating directly elsewhere.
             * \param r The associated Request
             * \param t The type of the message
             * \param payload_size The size in bytes of the data to be sent
             */
            NoCMessage(std::shared_ptr<Request> r, NoCMessageType t, uint64_t payload_size);
            

            /*!
             * \brief Get the type of the message
             * \return The type
             */
            NoCMessageType getType(){return message_type_;}
            
            /*!
             * \brief Get the Request associated to the message
             * \return The Request
             */
            std::shared_ptr<Request> getRequest(){return request_;}

            
            /*!
             * \brief Get the size of the message (including its header)
             * \return The size
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
