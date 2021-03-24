#ifndef __REQUEST_HH__
#define __REQUEST_HH__

#include "CacheDataMappingPolicy.hpp"
#include "Event.hpp"
#include <iostream>

namespace spike_model
{
    class Request : public Event
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
        friend class EventManager;

        public:
            enum class RegType
            {
                INTEGER,
                FLOAT,
                VECTOR,
            };

            //Request(){}
            Request() = delete;
            Request(Request const&) = delete;
            Request& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for Request
             * \param  a The requested address
             * \param  pc The program counter of the requesting instruction
             */
            Request(uint64_t a, uint64_t pc): Event(pc), address(a){}

            /*!
             * \brief Constructor for Request
             * \param a The requested address
             * \param pc The program counter of the requesting instruction
             * \param time The timestamp for the request
             * \param c The requesting core
             */
            Request(uint64_t a, uint64_t pc, uint64_t time, uint16_t c): Event(pc, time, c), address(a) {}

            /*!
             * \brief Get the address of the request
             * \return The address of the request
             */
            uint64_t getAddress() const {return address;}

            /*!
             * \brief Get the id of the destination register for the request
             * \return The id for the destination register
             */
            uint8_t getDestinationRegId() const {return regId;}

            /*!
             * \brief Set the size of the store
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

            /*!
             * \brief Set the id and type of the destination register for the request
             * \param r The id of the register
             * \param t The type of the register
             */
            void setDestinationReg(uint8_t r, RegType t) {regId=r; regType=t;}

            /*!
             * \brief Get the type of the destination register for the request
             * \return The type of the register
             */
            RegType getDestinationRegType() const {return regType;}


                bool operator ==(const Request & m) const
            {
                return m.getAddress()==getAddress();
            }


        private:
            uint64_t address;
            uint16_t cache_bank;

            uint8_t regId;
            RegType regType;

            uint16_t size;
    };

    inline std::ostream & operator<<(std::ostream & Str, Request const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }
}
#endif
