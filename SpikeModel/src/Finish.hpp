#ifndef __FINISH_HH__
#define __FINISH_HH__

#include "Sync.hpp"
#include "EventVisitor.hpp"
#include <iostream>

namespace spike_model
{
    class Finish : public Sync, public std::enable_shared_from_this<Finish>
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
            Finish(Finish const&) = delete;
            Finish& operator=(Finish const&) = delete;

            /*!
             * \brief Constructor for Finish
             * \param pc The program counter of the instruction that finished the simulation
             */
            Finish(uint64_t pc): Sync(pc){}

            /*!
             * \brief Constructor for Finish
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The finishing core
             */
            Finish(uint64_t pc, uint64_t time, uint16_t c): Sync(pc, time, c) {}

            
            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

    };
}
#endif
