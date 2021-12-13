#ifndef __CORE_EVENT_HH__
#define __CORE_EVENT_HH__

#include <iostream>
#include "Event.hpp"

namespace spike_model
{
    class CoreEvent : public spike_model::Event
    {
        /**
         * \class spike_model::CoreEvent
         *
         * \brief CoreEvent signals that an event related to a core has ocurred in Spike
         *
         */
        
        public:
            //CoreEvent(){}
            CoreEvent() = delete;
            CoreEvent(CoreEvent const&) = delete;
            CoreEvent& operator=(CoreEvent const&) = delete;

            /*!
             * \brief Constructor for CoreEvent
             * \param pc The program counter of the instruction that finished the simulation
             */
            CoreEvent(uint64_t pc): Event(), pc(pc){}

            /*!
             * \brief Constructor for CoreEvent
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            CoreEvent(uint64_t pc, uint64_t time, uint16_t c): Event(time), pc(pc), coreId(c){}
            
            /*!
             * \brief Set the source tile of the event
             * \param source The source tile
             */
            void setSourceTile(uint16_t source)
            {
                source_tile=source;
            }

            /*!
             * \brief Get the source tile for the event
             * \return The source tile
             */
            uint16_t getSourceTile(){return source_tile;}

            /*!
             * \brief Get the eventing core
             * \return The core id
             */
            uint64_t getCoreId() const {return coreId;}

            /*!
             * \brief Set the eventing core
             * \param c The id of the core
             */
            void setCoreId(uint16_t c) {coreId=c;}

            /*!
             * \brief Get: the program counter for the eventing instruction
             * \return The PC
             */
            uint64_t getPC(){return pc;}

            
        private:
            uint64_t pc;
            uint16_t coreId;
            uint16_t source_tile;
    };
}
#endif
