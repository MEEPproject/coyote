#ifndef __EVENT_HH__
#define __EVENT_HH__

#include "EventVisitor.hpp"

namespace spike_model
{
    class EventVisitor;
    class Event
    {
        /**
         * \class spike_model::Event
         *
         * \brief Event contains all the information regarding an event to be communicated from Spike to the rest of Coyote.
         *
         * Instances of Event (ant its children) are the main data structure that is communicated between Spike and the Sparta model
         * and between Sparta Units.
         *
         */
        
        public:

            Event() = delete;
            Event(Event const&) = delete;
            Event& operator=(Event const&) = delete;

            /*!
             * \brief Constructor for Event
             * \param  pc The program counter of the requesting instruction
             */
            Event(uint64_t pc, uint16_t c): pc(pc), coreId(c){}

            /*!
             * \brief Constructor for Event
             * \param pc The program counter of the requesting instruction
             * \param time The timestamp for the request
             * \param c The producing core
             */
            Event(uint64_t pc, uint64_t time, uint16_t c): pc(pc), timestamp(time), coreId(c){}

            /*!
             * \brief Constructor for Event
             * \param c The producing core
             */
            Event(uint16_t c): coreId(c){}

            /*!
             * \brief Set the source tile of the request
             * \param source The source tile
             */
            void setSourceTile(uint16_t source)
            {
                source_tile=source;
            }

            /*!
             * \brief Get the source tile for the request
             * \return The source tile
             */
            uint16_t getSourceTile(){return source_tile;}

            /*!
             * \brief Get the timestamp of the event
             * \return The timestamp
             */
            uint64_t getTimestamp() const {return timestamp;}

            /*!
             * \brief Get the requesting core
             * \return The core id
             */
            uint64_t getCoreId() const {return coreId;}

            /*!
             * \brief Set the timestamp of the request
             * \param t The timestamp
             */
            void setTimestamp(uint64_t t) {timestamp=t;}

            /*!
             * \brief Set the requesting core
             * \param c The id of the core
             */
            void setCoreId(uint16_t c) {coreId=c;}

            /*!
             * \brief Get: the program counter for the requesting instruction
             * \return The PC
             */
            uint64_t getPC(){return pc;}

            /*!
             * \brief Get whether the request has been serviced
             * \return True if the request has been serviced
             */
            bool isServiced(){return serviced;}

            /*!
             * \brief Set the request as serviced
             */
            void setServiced(){serviced=true;}

            /*
             * \brief Equality operator for instances of Request
             * \param m The request to compare with
             * \return true if both requests are for the same address
             */

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            virtual void handle(EventVisitor * v)=0;

        private:
            uint64_t pc;
            uint64_t timestamp;
            uint16_t coreId;
            uint16_t source_tile;
            bool serviced=false;
    };
    
/*    inline std::ostream & operator<<(std::ostream & Str, Request const & req)
    {
        Str << "0x" << std::hex << req.getAddress() << " @ " << req.getTimestamp();
        return Str;
    }*/
}
#endif
