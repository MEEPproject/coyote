#ifndef __MCPU_INSTRUCTION_HH__
#define __MCPU_INSTRUCTION_HH__

#include "Request.hpp"
#include "VectorElementType.hpp"

#include <iostream>
#include <vector>


namespace spike_model {
	class MCPUInstruction : public Request, public std::enable_shared_from_this<MCPUInstruction> {
		/**
		 * \class spike_model::Fence
		 *
		 * \brief Fence signals that the Spike simulation has been completed.
		 *
		 */


		public:
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
			MCPUInstruction(uint64_t pc, uint64_t addr, Operation o, VectorElementType w):Request(addr, pc), operation(o), width(w) {}

			/*!
			 * \brief Constructor for memory instruction that will be handled by the MCPU
			 * \param pc The program counter of the instruction that finished the simulation
			 * \param time The timestamp for the finish
			 * \param c The core generating the instruction
			 * \param addr The base address for the memory instruction
			 * \param o The type of operation
			 * \param w The width of the vector element
			 */
			MCPUInstruction(uint64_t pc, uint64_t time, uint16_t c, uint64_t addr, Operation o, VectorElementType w): Request(addr, pc, time, c), operation(o), width(w) {}

			/*!
			 * \brief Handle the event
			 * \param v The visitor to handle the event
			 */
			void handle(EventVisitor * v) override {
				v->handle(shared_from_this());
			}


			void setIndexed(std::vector<uint64_t> indices_) {indices=indices_; sub_operation = SubOperation::UNORDERED_INDEX;}
			void setOrdered(std::vector<uint64_t> indices_) {indices=indices_; sub_operation = SubOperation::ORDERED_INDEX;}
			void setStride(std::vector<uint64_t> indices_)  {indices=indices_; sub_operation = SubOperation::NON_UNIT;}
			std::vector<uint64_t> get_index() {return indices;}
						
			VectorElementType get_width() {return width;}
			void set_width(VectorElementType width_) {width = width_;}

			Operation get_operation() {return operation;}
			SubOperation get_suboperation() {return sub_operation;}



		private:
			std::vector<uint64_t> indices;
			Operation operation;
			SubOperation sub_operation = SubOperation::UNIT;
			VectorElementType width;		 // 8, 16, 32, or 64 bit memory operation
	};
	
	inline std::ostream& operator<<(std::ostream &str, MCPUInstruction &instr) {
		str << "0x" << std::hex << instr.getAddress() << " @ " << instr.getTimestamp() << " Op: 0x" << (uint)instr.get_operation() << " SubOp: 0x" << (uint)instr.get_suboperation() << ", width: 0x" << (uint)instr.get_width() << ", coreID: 0x" << instr.getCoreId() << ", destReg: 0x" << instr.getDestinationRegId() << ", ID: 0x" << instr.getID() << ", PC: " << instr.getPC();
		return str;
	}
}
#endif
