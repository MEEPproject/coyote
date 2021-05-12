#ifndef __MCPU_INSTRUCTION_HH__
#define __MCPU_INSTRUCTION_HH__

#include "EventVisitor.hpp"
#include <iostream>
#include <vector>

namespace spike_model {
    class MCPUInstruction : public Event, public std::enable_shared_from_this<MCPUInstruction> {
        /**
         * \class spike_model::Fence
         *
         * \brief Fence signals that the Spike simulation has been completed.
         *
         */


        public:

            enum class Width {
                BIT8, BIT16, BIT32, BIT64 //The change of this enums should not be changed. The correct assignation of the width in riscv-isa-sim depends on it
            };

            enum class Stride {
                UNIT, NON_UNIT, INDEXED
            };

            enum class Operation {
                LOAD, STORE
            };


            //Fence(){}
            MCPUInstruction() = delete;
            MCPUInstruction(MCPUInstruction const&) = delete;
            MCPUInstruction& operator=(MCPUInstruction const&) = delete;

            /*!
             * \brief Constructor for memory instruction that will be handled by the MCPU
             * \param pc The program counter of the instruction that finished the simulation
             * \param addr The base address for the memory instruction
             * \param o The type of operation
             * \param w The width of the vector element
             */
            MCPUInstruction(uint64_t pc, uint64_t addr, Operation o, Width w):Event(pc), base_address(addr), operation(o), width(w){}

            /*!
             * \brief Constructor for memory instruction that will be handled by the MCPU
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp for the finish
             * \param c The core generating the instruction
             * \param addr The base address for the memory instruction
             * \param o The type of operation
             * \param w The width of the vector element
             */
            MCPUInstruction(uint64_t pc, uint64_t time, uint16_t c, uint64_t addr, Operation o, Width w): Event(pc, time, c), base_address(addr), operation(o), width(w){}

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override {
                v->handle(shared_from_this());
            }

            uint64_t getBaseAddress() {return base_address;}

            std::vector<uint64_t> getIndex() {
                return indices;
            }

            //-- setters and getters
            bool getIndexed() {return indexed;}
            void setIndexed(std::vector<uint64_t> i) {indices=i; stride = Stride::INDEXED; indexed=true;}

            bool getSegmented() {return segmented;}
            void setSegmented(bool segmented_) {segmented = segmented_;}

            bool getOrdered() {return ordered;}
            void setOrdered(bool ordered_) {ordered = ordered_;}

            Stride getStride() {return stride;}
            void setStride(std::vector<uint64_t> i) {indices=i; stride = Stride::NON_UNIT;}

            Width getWidth() {return width;}
            void setWidth(Width width_) {width = width_;}

            Operation getOperation() {return operation;}
            void setOperation(Operation operation_) {operation = operation_;}

        private:
            uint64_t base_address;
            std::vector<uint64_t> indices;
            Operation operation;    // load = true, store = false
            Width width;            // 8, 16, 32, or 64 bit memory operation
            bool ordered=false;           // are the indices in a particular order (or can they be in any order?)
            bool segmented=false;         // segmented load/store
            bool indexed=false;           // indexed load/store
            Stride stride=Stride::UNIT;   // stride
    };
}
#endif
