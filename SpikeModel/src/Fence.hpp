#ifndef __FENCE_HH__
#define __FENCE_HH__

#include "SpikeEvent.hpp"
#include "SpikeEventVisitor.hpp"
#include <iostream>

namespace spike_model
{
    class Fence : public SpikeEvent, public std::enable_shared_from_this<Fence>
    {
        /**
         * \class spike_model::Fence
         *
         * \brief Fence signals that the Spike simulation has been completed.
         *
         */
        
        public:

            //Fence(){}
            Fence() = delete;
            Fence(Request const&) = delete;
            Fence& operator=(Request const&) = delete;

            /*!
             * \brief Constructor for Fence
             * \param pc The program counter of the instruction that finished the simulation
             */
            Fence(uint64_t pc): SpikeEvent(pc){}

            /*!
             * \brief Constructor for Fence
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            Fence(uint64_t pc, uint64_t time, uint16_t c): SpikeEvent(pc, time, c) {}

            
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
