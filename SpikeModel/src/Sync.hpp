#ifndef __SYNC_HH__
#define __SYNC_HH__

#include <iostream>
#include "Event.hpp"

namespace spike_model
{
    class Sync : public spike_model::Event
    {
        /**
         * \class spike_model::Sync
         *
         * \brief Sync signals that the Spike simulation has been completed.
         *
         */
        
        public:

            //Sync(){}
            Sync() = delete;
            Sync(Sync const&) = delete;
            Sync& operator=(Sync const&) = delete;

            /*!
             * \brief Constructor for Sync
             * \param pc The program counter of the instruction that finished the simulation
             */
            Sync(uint64_t pc): Event(pc){}

            /*!
             * \brief Constructor for Sync
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            Sync(uint64_t pc, uint64_t time, uint16_t c): Event(pc, time, c) {}
    };
}
#endif
