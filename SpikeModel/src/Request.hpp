#ifndef __REQUEST_HH__
#define __REQUEST_HH__

#include "CacheDataMappingPolicy.hpp"
#include "RegisterEvent.hpp"
#include "ParentInstId.hpp"
#include <iostream>

namespace spike_model
{
    class Request : public RegisterEvent, public ParentInstId
    {
        /**
         * \class spike_model::Request
         *
         * \brief Request contains all the information regarding a request to the L2 cache.
         *
         * Instances of request are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units, either as plain Requests or encapsulated in a NoCMessage.
         *
         */
        friend class FullSystemSimulationEventManager;

        public:

            //Request(){}
            Request() = delete;
            Request(Request const&) = delete;
            Request& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for Request
             * \param a The requested address
             * \param pc The program counter of the requesting instruction
             * \param c The requesting core
             */
            Request(uint64_t a, uint64_t pc, uint16_t c): RegisterEvent(pc, 0, c, -1, spike_model::RegisterEvent::RegType::DONT_CARE), ParentInstId(), address(a){}

            /*!
             * \brief constructor for request
             * \param a the requested address
             * \param pc the program counter of the requesting instruction
             * \param time the timestamp for the request
             * \param c the requesting core
             */
            Request(uint64_t a, uint64_t pc, uint64_t time, uint16_t c): RegisterEvent(pc, time, c, -1, spike_model::RegisterEvent::RegType::DONT_CARE), ParentInstId(), address(a) {}

            /*!
             * \brief Constructor for Request
             * \param coreId The requested address
             * \param regId The id of the destination register for the request
             * \param regType The type of the destination register for the request
             */
            Request(uint16_t coreId, uint64_t regId, spike_model::RegisterEvent::RegType regType):
                             RegisterEvent(0, 0, coreId, regId, regType), ParentInstId() {}

            Request(uint16_t coreId): RegisterEvent(0, 0, coreId, -1, spike_model::RegisterEvent::RegType::DONT_CARE), ParentInstId() {}

            Request(uint16_t coreId, uint64_t regId): RegisterEvent(0, 0, coreId, -1, spike_model::RegisterEvent::RegType::DONT_CARE), ParentInstId() {}

            /*!
             * \brief Get the address of the request
             * \return The address of the request
             */
            uint64_t getAddress() const {return address;}

            /*!
             * \brief Set the size of the request in bytes
             * \param s The size
             */
            void setSize(uint16_t s) {size=s;}

            /*!
             * \brief Get the size of the store
             * \return The size of the store
             */
            uint16_t getSize() const {return size;}

            /*!
             * \brief Set the cache bank that will be accessed by the request
             * \param b The cache bank
             */
            void setCacheBank(uint16_t b)
            {
                cache_bank=b;
            }

            /*!
             * \brief Get the bank that will be accessed by the request
             * \return The bank
             */
            uint16_t getCacheBank(){return cache_bank;}
 
            
            bool operator ==(const Request & m) const
            {
                return m.getAddress()==getAddress();
            }


        private:
            uint64_t address;
            uint16_t cache_bank;
 
            uint16_t size;
    };

    inline std::ostream & operator<<(std::ostream & Str, Request const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }
}
#endif
