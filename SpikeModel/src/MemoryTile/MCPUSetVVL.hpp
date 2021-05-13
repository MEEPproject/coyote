#ifndef __MCPU_SetVVL_HH__
#define __MCPU_SetVVL_HH__

#include "EventVisitor.hpp"
#include <iostream>

namespace spike_model
{
    class MCPUSetVVL : public Event, public std::enable_shared_from_this<MCPUSetVVL>
    {
        /**
         * \class spike_model::MCPUSetVVL
         *
         * \brief MCPUSetVVL passes the virtual vector length to MCPU.
         *
         */

        public:

            MCPUSetVVL() = delete;
            MCPUSetVVL(MCPUSetVVL const&) = delete;
            MCPUSetVVL& operator=(MCPUSetVVL const&) = delete;

            /*!
             * \brief Constructor for MCPUSetVVL
             * \param pc The program counter of the instruction that generates the event
             */
            MCPUSetVVL(uint64_t pc):Event(pc)
            {}

            /*!
             * \brief Constructor for MCPUSetVVL
             * \param VVL The virtual vector length set by scalar core
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp when the event is submitted to sparta
             * \param c The core which submitted the event
             */
            MCPUSetVVL(uint64_t VVL, uint64_t pc, uint64_t time, uint16_t c): Event(pc, time, c)
                          , VVL(VVL){}

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

            uint64_t getVVL()
            {
                return VVL;
            }

            void setVVL(uint64_t VVL)
            {
                this->VVL = VVL;
            }

            /*!
             * \brief Get the memory CPU that will be accessed
             * \return The address of the line              */
            uint64_t getMemoryCPU()
            {
                return memoryCPU;
            }

        private:
            uint64_t VVL, memoryCPU;
    };
}
#endif
