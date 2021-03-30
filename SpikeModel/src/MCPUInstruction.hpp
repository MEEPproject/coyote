#ifndef __MCPU_INSTRUCTION_HH__
#define __MCPU_INSTRUCTION_HH__

#include "EventVisitor.hpp"
#include <iostream>
#include <vector>

namespace spike_model
{
    class MCPUInstruction : public Event, public std::enable_shared_from_this<MCPUInstruction>
    {
        /**
         * \class spike_model::Fence
         *
         * \brief Fence signals that the Spike simulation has been completed.
         *
         */

        public:

            //Fence(){}
            MCPUInstruction() = delete;
            MCPUInstruction(MCPUInstruction const&) = delete;
            MCPUInstruction& operator=(MCPUInstruction const&) = delete;

            /*!
             * \brief Constructor for memory instruction that will be handled by the MCPU
             * \param pc The program counter of the instruction that finished the simulation
             * \param addr The base address for the memory instruction
             */
            MCPUInstruction(uint64_t pc, uint64_t addr):Event(pc), base_address(addr), indices()
            {}

            /*!
             * \brief Constructor for memory instruction that will be handled by the MCPU
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The core generating the instruction
             * \param addr The base address for the memory instruction
             */
            MCPUInstruction(uint64_t pc, uint64_t time, uint16_t c, uint64_t addr): Event(pc, time, c), base_address(addr), indices()
            {}

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override
            {
                v->handle(shared_from_this());
            }

            uint64_t getBaseAddress()
            {
                return base_address;
            }

            std::vector<uint64_t> getIndex()
            {
                return indices;
            }

            void addIndex(uint64_t index)
            {
                indices.push_back(index);
            }

        private:
            uint64_t base_address;
            std::vector<uint64_t> indices;

    };
}
#endif
