#ifndef __FINISH_HH__
#define __FINISH_HH__

#include "SpikeEvent.hpp"
#include "SpikeEventVisitor.hpp"
#include <iostream>

namespace spike_model
{
    class Finish : public SpikeEvent, public std::enable_shared_from_this<Finish>
    {
        /**
         * \class spike_model::Finish
         *
         * \brief Finish signals that the Spike simulation has been completed.
         *
         */
        
        public:

            //Finish(){}
            Finish() = delete;
            Finish(Request const&) = delete;
            Finish& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for Finish
             * \param pc The program counter of the instruction that finished the simulation
             */
            Finish(uint64_t pc): SpikeEvent(pc){}

            /*!
             * \brief Constructor for Finish
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            Finish(uint64_t pc, uint64_t time, uint16_t c): SpikeEvent(pc, time, c) {}

            
            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(SpikeEventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

    };
}
#endif
