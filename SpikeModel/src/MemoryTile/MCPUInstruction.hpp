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
				BIT8=1, BIT16=2, BIT32=4, BIT64=8 //The order of this enum should not be changed. The correct assignation of the width in riscv-isa-sim depends on it
			};

			enum class Operation {
				LOAD,
				STORE
			};
			
			enum class SubOperation {
				UNIT,
				NON_UNIT,
				ORDERED_INDEX,
				UNORDERED_INDEX
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

			void set_baseAddress(uint64_t address) {base_address = address;}
			uint64_t get_baseAddress() {return base_address;}

			void setIndexed(std::vector<uint64_t> indices_) {indices=indices_; sub_operation = SubOperation::UNORDERED_INDEX;}
			void setOrdered(std::vector<uint64_t> indices_) {indices=indices_; sub_operation = SubOperation::ORDERED_INDEX;}
			void setStride(std::vector<uint64_t> indices_)  {indices=indices_; sub_operation = SubOperation::NON_UNIT;}
			std::vector<uint64_t> get_index() {return indices;}
						
			Width get_width() {return width;}
			void set_width(Width width_) {width = width_;}

			Operation get_operation() {return operation;}
			SubOperation get_suboperation() {return sub_operation;}

			uint32_t  getMCPUInstruction_ID();{return  MCPUInstruction_ID ;}

            void setMCPUInstruction_ID(uint32_t instr_id);{MCPUInstruction_ID = instr_id;}

		private:
		    uint32_t MCPUInstruction_ID;
			uint64_t base_address;
			std::vector<uint64_t> indices;
			Operation operation;
			SubOperation sub_operation = SubOperation::UNIT;
			Width width;            // 8, 16, 32, or 64 bit memory operation
	};
}
#endif
