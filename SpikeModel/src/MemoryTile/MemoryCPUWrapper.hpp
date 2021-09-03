#ifndef __MEMORY_CPU_WRAPPER_H__
#define __MEMORY_CPU_WRAPPER_H__


#include "sparta/ports/DataPort.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"
#include "sparta/app/Simulation.hpp"

#include "LogCapable.hpp"
#include "NoC/NoCMessage.hpp"
#include "NoC/NoC.hpp"
#include "Event.hpp"
#include "EventManager.hpp"
#include "MCPUSetVVL.hpp"
#include "MCPUInstruction.hpp"
#include "CacheRequest.hpp"
#include "ScratchpadRequest.hpp"
#include "Bus.hpp"
#include <unordered_map>

namespace spike_model {
	class MemoryCPUWrapper : public sparta::Unit, public LogCapable, public spike_model::EventVisitor {
		public:

			using spike_model::EventVisitor::handle; //This prevents the compiler from warning on overloading


			/*!
			 * \class MemoryCPUWrapperParameterSet
			 * \brief Parameters for MemoryCPUWrapper model
			 */
			class MemoryCPUWrapperParameterSet : public sparta::ParameterSet {
				public:
					//! Constructor for MemoryCPUParameterSet
					MemoryCPUWrapperParameterSet(sparta::TreeNode* n):sparta::ParameterSet(n) { }
					PARAMETER(uint64_t, line_size, 64, "The cache line size")
					PARAMETER(uint64_t, latency, 1, "The latency of the buses in the memory CPU wrapper")
			};

			/*!
			 * \brief Constructor for MemoryCPUWrapper
			 * \param  node parameter is the node that represents the MemoryCPUWrapper
			 * \param p is the MemoryCPUWrapper parameter set
			 */
			MemoryCPUWrapper(sparta::TreeNode* node, const MemoryCPUWrapperParameterSet* p);

			~MemoryCPUWrapper() {
				debug_logger_ << getContainer()->getLocation() << ": " << std::endl;
			}

			//! name of this resource.
			static const char name[];

			void notifyCompletion_();

			/*!
			 * \brief Sets the request manager for the tile
			 */
			void setRequestManager(std::shared_ptr<EventManager> r);
			
			/*!
			 * \brief Set the ID for the Memory Tile
			 * \param The ID to be set
			 */
			void setID(uint16_t id) {this->id = id;}
			
			/*!
			 * \brief Get the ID of the current Memory Tile
			 * \return The ID of this Memory Tile
			 */
			uint16_t getID() {return this->id;}

			/*!
			 * \brief Set the pointer to the NoC, which is then used
			 * in the MCPUWrapper to access methods (such as checking
			 * if a Transaction can be accepted) inside the NoC.
			 * \param noc A pointer to the NoC
			 */
			void setNoC(spike_model::NoC *noc) {this->noc = noc;}

            /*!
             * \brief Set the information on the address mapping
             * \param memory_controller_shift The number of bits to shift to get the memory controller that handles an address
             * \param memory_controller_mask The mask for the AND to extract the memory controller id
             */
            void setAddressMappingInfo(uint64_t memory_controller_shift, uint64_t memory_controller_mask);

		private:
			const uint8_t num_of_registers = 32;
			size_t sp_reg_size;

			enum class SPStatus {
				IS_L2 = 0,
				ALLOC_SENT,
				READY
			};
			
			SPStatus *sp_status;
			
			uint16_t id;
			uint32_t line_size_;
			uint32_t vvl_;
			uint64_t latency_;
			uint32_t instructionID_counter; // ID issued to incoming mcpu instructions. increments with every new instruction
			bool enabled;
		
            uint64_t mc_shift;
            uint64_t mc_mask;
			
			struct transaction {
				std::shared_ptr<MCPUInstruction> mcpu_instruction;
				uint32_t counter_cacheRequests;
				uint32_t counter_scratchpadRequests;
				uint32_t number_of_elements_per_response;
			};
			std::unordered_map<std::uint32_t, transaction> transaction_table;
			
			

			//-- message handling 
			sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc_ {
				&unit_port_set_, "out_noc"
			};

			sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc_ {
				&unit_port_set_, "in_noc"
			};

			sparta::DataOutPort<std::shared_ptr<CacheRequest>> out_port_mc_ {
				&unit_port_set_, "out_mc"
			};

			sparta::DataInPort<std::shared_ptr<CacheRequest>> in_port_mc_ {
				&unit_port_set_, "in_mc"
			};
			

			//-- The Memory Tile Bus (for Memory requests from VAG,bypass...)
			sparta::UniqueEvent<sparta::SchedulingPhase::Tick> controller_cycle_event_mem_req {
			   &unit_event_set_, "controller_cycle_mem_requests", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, controllerCycle_mem_requests)
			}; 
			Bus<std::shared_ptr<CacheRequest>> sched_mem_req;

			
			//-- Bus for outgoing transactions
			sparta::UniqueEvent<sparta::SchedulingPhase::Tick> controller_cycle_event_outgoing_transaction {
					&unit_event_set_, "controller_cycle_outgoing_transaction", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, controllerCycle_outgoing_transaction)
			};
			BusDelay<std::shared_ptr<NoCMessage>> sched_outgoing;

			
			//-- Bus for incoming transactions
			sparta::UniqueEvent<sparta::SchedulingPhase::Tick> controller_cycle_event_incoming_transaction {
					&unit_event_set_, "controller_cycle_incoming_transaction", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, controllerCycle_incoming_transaction)
			};
			Bus<std::shared_ptr<Request>> sched_incoming;
			
			


			std::shared_ptr<EventManager> request_manager_;
			spike_model::NoC *noc;
			sparta::RootTreeNode *root_node;
			
			
			void receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes);
			void receiveMessage_mc_(const std::shared_ptr<CacheRequest> &mes);
			
			/*!
			 * \brief Handles an instruction forwarded to the MCPU
			 * \param r The instruction to handle
			 */
			virtual void handle(std::shared_ptr<spike_model::CacheRequest> r) override;
			virtual void handle(std::shared_ptr<spike_model::MCPUSetVVL> r) override;
			virtual void handle(std::shared_ptr<spike_model::MCPUInstruction> r) override;
			virtual void handle(std::shared_ptr<spike_model::ScratchpadRequest> r) override;
			

			void controllerCycle_incoming_transaction();	// for incoming MCPUInstructions (NoC -> MemTile)
			void controllerCycle_outgoing_transaction();	// Outgoing transaction queue (MemTile -> NoC)
			void controllerCycle_mem_requests(); 			// Bus for MemTile -> MC)
			void memOp_unit(std::shared_ptr<MCPUInstruction> instr);
			void memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr);
			void memOp_orderedIndex(std::shared_ptr<MCPUInstruction> instr);
			void memOp_unorderedIndex(std::shared_ptr<MCPUInstruction> instr);
			std::shared_ptr<ScratchpadRequest> createScratchpadRequest(const std::shared_ptr<Request> &mes, ScratchpadRequest::ScratchpadCommand command);
			std::shared_ptr<CacheRequest> createCacheRequest(uint64_t address, std::shared_ptr<MCPUInstruction> instr);
			void computeMemReqAddresses(std::shared_ptr<MCPUInstruction> instr);
			uint16_t calcDestMemTile(uint64_t address);
			


			//-- reporting and logging
			sparta::Counter count_requests_noc_=sparta::Counter(getStatisticSet(), "requests_noc", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_load_=sparta::Counter(getStatisticSet(), "requests_noc_load", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_store_=sparta::Counter(getStatisticSet(), "requests_noc_store", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_requests_mc_=sparta::Counter(getStatisticSet(), "requests_mc", "Number of requests from MC", sparta::Counter::COUNT_NORMAL);
	};
}
#endif
