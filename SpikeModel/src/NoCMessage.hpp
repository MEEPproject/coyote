
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
            
            /*!
             * \brief Constructor for NoCMessage
             * \note  r is the associated L2Request
             *        t is the type of the message
             *        payload_size is the size in bytes of the data to be sent
             */
            NoCMessage(std::shared_ptr<L2Request> r, NoCMessageType t, uint64_t payload_size);

            /*!
             * \brief Returns the type of the message
             */
            NoCMessageType getType(){return message_type_;}
            
            /*!
             * \brief Returns a smart pointer to the L2Request associated to the message
             */
            std::shared_ptr<L2Request> getRequest(){return request_;}

            /*!
             * \brief Returns the memory controller that corresponds to this message
             * \note num_controllers is the number of memory controllers in the simulated system
             *       Generates an error if the message is not a memory request
             */
            uint16_t getMemoryController(uint16_t num_controllers);
            
            /*!
             * \brief Returns the size of the message including its header
             */
            uint64_t getSize();

        private:
            std::shared_ptr<L2Request> request_;
            NoCMessageType message_type_;
            uint64_t size_;

            //The size of message headers in bytes. TODO: MAKE IT A PARAMETER
            static const uint8_t header_size_=1;
    };
}
#endif
