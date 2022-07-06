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
#include "FullSystemSimulationEventManager.hpp"
#include "MCPUSetVVL.hpp"
#include "MCPUInstruction.hpp"
#include "CacheRequest.hpp"
#include "ScratchpadRequest.hpp"
#include "CacheDataMappingPolicy.hpp"
#include "Bus.hpp"
#include <unordered_map>

namespace coyote {
		
	class NoC; // forward declaration
	
	class MemoryCPUWrapper : public sparta::Unit, public LogCapable, public coyote::EventVisitor {
		public:

			using coyote::EventVisitor::handle; //This prevents the compiler from warning on overloading


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
					PARAMETER(uint16_t, num_llc_banks, 1, "The number of llc cache banks in the memory tile")
					PARAMETER(std::string, llc_pol, "set_interleaving", "The data mapping policy for banks")
					PARAMETER(uint32_t, max_vvl, 65536, "The maximum vvl that the MCPU will return")
			};

			/*!
			 * \brief Constructor for MemoryCPUWrapper
			 * \param  node parameter is the node that represents the MemoryCPUWrapper
			 * \param p is the MemoryCPUWrapper parameter set
			 */
			MemoryCPUWrapper(sparta::TreeNode* node, const MemoryCPUWrapperParameterSet* p);

			~MemoryCPUWrapper() {
				transaction_table.clear();
			}

			//! name of this resource.
			static const char name[];

			/*!
			 * \brief Set the ID for the Memory Tile
			 * \param The ID to be set
			 */
			void setID(uint16_t id);

			/*!
			 * \brief Get the ID of the current Memory Tile
			 * \return The ID of this Memory Tile
			 */
			uint16_t getID();

			/*!
			 * \brief Set the pointer to the NoC, which is then used
			 * in the MCPUWrapper to access methods (such as checking
			 * if a Transaction can be accepted) inside the NoC.
			 * \param noc A pointer to the NoC
			 */
			void setNoC(coyote::NoC *noc) {this->noc = noc;}

			/*!
			 * \brief Set the information on the address mapping
			 * \param memory_controller_shift The number of bits to shift to get the memory controller that handles an address
			 * \param memory_controller_mask The mask for the AND to extract the memory controller id
			 */
			void setAddressMappingInfo(uint64_t memory_controller_shift, uint64_t memory_controller_mask);

			/*!
             * \brief Set the addressing information for the LLC
             * \param size_kbs The total size of the LLC for this memory tile in KBs
             * \param assoc The associativity of the LLC
             */
			void setLLCInfo(uint64_t size_kbs, uint8_t assoc);
			
			
			/*!
			 * \brief This method allows to inform the NoC, if a packet in [mes] can be accepted. This allows the MemoryTile
			 * to simulate multiple NoC ports. For instance, if the Memory Tile can handle only one vector memory operation at
			 * a time. However, some of those memory operation require additional information such as indices. If the MemorTile
			 * becomes busy, no other packets, including those carrying the indices, can enter the memory tile. Therefore, this
			 * method allows to "peek" into the next packet to be sent, and can then deny it. The NoC will retry to send the
			 * packet in the future.
			 * This method is called $n$ times per simulated clock cycle. $n$ is equal to the number of NoCs. Therefore, other
			 * packets traversing on another pane/NoC will be able to overtake the stalled packet. This scenario prevents deadlocks
			 * when, e.g. a second vector memory instruction is stalled, but the current one still waits for LVRF packets from the
			 * VAS Tile.
			 * \param mes The NoC message
			 * \return True, if the memory tile can accept the packet offered by the NoC, false otherwise.
			 */
			bool ableToReceivePacket(const std::shared_ptr<NoCMessage> &mes);

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
			uint32_t line_size;
			uint32_t *vvl;
			uint64_t latency;
			uint16_t llc_banks;
			CacheDataMappingPolicy llc_policy;
			uint32_t max_vvl;
			uint32_t instructionID_counter; // ID issued to incoming mcpu instructions. increments with every new instruction
			bool enabled;
			bool enabled_llc;
		
			uint64_t mc_shift;
			uint64_t mc_mask;
			
			uint8_t tag_bits;
			uint8_t block_offset_bits;
			uint8_t set_bits;
			uint8_t bank_bits;
			
			struct LastLogTime {
				uint64_t sched_outgoing = 0;
				uint64_t sched_mem_req = 0;
			};
			LastLogTime lastLogTime;
			
			struct Transaction {
				std::shared_ptr<MCPUInstruction> mcpu_instruction;
				uint32_t counter_cacheRequests;
				uint32_t counter_scratchpadRequests;
				uint32_t number_of_elements_per_response;
				uint32_t vvl;	// the VVL setting, when the instruction arrived at the MemTile
			};
			std::unordered_map<std::uint32_t, Transaction> transaction_table;
			
			

			//-- message handling 
			sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc {
				&unit_port_set_, "out_noc"
			};

			sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc {
				&unit_port_set_, "in_noc"
			};

			sparta::DataOutPort<std::shared_ptr<CacheRequest>> out_port_mc {
				&unit_port_set_, "out_mc", false 
			};

			sparta::DataInPort<std::shared_ptr<CacheRequest>> in_port_mc {
				&unit_port_set_, "in_mc"
			};
			
			std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<Request>>>> out_ports_llc;
			std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<Request>>>> in_ports_llc;
			std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<CacheRequest>>>> out_ports_llc_mc;
			std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<CacheRequest>>>> in_ports_llc_mc;
		
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
			sparta::UniqueEvent<sparta::SchedulingPhase::Tick> controller_cycle_event_incoming_mem_req {
					&unit_event_set_, "controller_cycle_incoming_mem_req", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, controllerCycle_incoming_mem_req)
			};
			Bus<std::shared_ptr<CacheRequest>> sched_incoming_mc;
			
			
			
			


			//std::shared_ptr<FullSystemSimulationEventManager> request_manager;
			coyote::NoC *noc;
			sparta::RootTreeNode *root_node;
			
			
			void receiveMessage_noc(const std::shared_ptr<NoCMessage> &mes);
			void receiveMessage_mc(const std::shared_ptr<CacheRequest> &mes);
			void receiveMessage_llc(const std::shared_ptr<CacheRequest> &mes);
			void receiveMessage_llc_mc(const std::shared_ptr<CacheRequest> &mes);
			
			/*!
			 * \brief Handles an instruction forwarded to the MCPU
			 * \param r The instruction to handle
			 */
			virtual void handle(std::shared_ptr<coyote::CacheRequest> r) override;
			virtual void handle(std::shared_ptr<coyote::MCPUSetVVL> r) override;
			virtual void handle(std::shared_ptr<coyote::MCPUInstruction> r) override;
			virtual void handle(std::shared_ptr<coyote::ScratchpadRequest> r) override;
			

			void controllerCycle_outgoing_transaction();					// Outgoing transaction queue (MemTile -> NoC)
			void controllerCycle_mem_requests(); 							// Bus for MemTile -> MC)
			void controllerCycle_incoming_mem_req();						// Bus MC/LLC -> MemTile

			void memOp_unit(std::shared_ptr<MCPUInstruction> instr);
			void memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr);
			void memOp_orderedIndex(std::shared_ptr<MCPUInstruction> instr);
			void memOp_unorderedIndex(std::shared_ptr<MCPUInstruction> instr);

			//This two functions might be fused into one with a better class hierarchy
			std::shared_ptr<ScratchpadRequest> createScratchpadRequest(const std::shared_ptr<Request> &mes, ScratchpadRequest::ScratchpadCommand command);
			std::shared_ptr<ScratchpadRequest> createScratchpadRequest(const std::shared_ptr<MCPUInstruction> &mes, ScratchpadRequest::ScratchpadCommand command);

			std::shared_ptr<CacheRequest> createCacheRequest(uint64_t address, std::shared_ptr<MCPUInstruction> instr);
			void computeMemReqAddresses(std::shared_ptr<MCPUInstruction> instr);
			uint16_t calcDestMemTile(uint64_t address);
			void handleReplyMessageFromMC(std::shared_ptr<CacheRequest> mes);
			void sendToDestination(std::shared_ptr<CacheRequest> mes);
			void log_sched_mem_req();
			void log_sched_outgoing();
			uint64_t getParentAddress(std::shared_ptr<CacheRequest> cr);
			

			uint16_t calculateBank(std::shared_ptr<coyote::CacheRequest> r);

			//-- reporting and logging
			sparta::Counter count_requests_noc 				= sparta::Counter(
					getStatisticSet(),
					"requests_noc",
					"Number of requests from NoC",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_replies_noc 				= sparta::Counter(
					getStatisticSet(),
					"replies_noc",
					"Number of replies given to the NoC",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_replies_wait_noc 			= sparta::Counter(
					getStatisticSet(),
					"replies_wait_noc",
					"Number of replies not accepted by the NoC",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_vector  					= sparta::Counter(
					getStatisticSet(),
					"vector_operations",
					"Number of vector operations",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_scalar 					= sparta::Counter(
					getStatisticSet(),
					"scalar_operations",
					"Number of scalar operations",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_control 					= sparta::Counter(
					getStatisticSet(),
					"control_operations",
					"Number of control operations (like vsetvl)",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_sp_requests				= sparta::Counter(
					getStatisticSet(),
					"sp_requests",
					"Number of SP requests (e.g. containing data) to the MemTile",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_send_other_memtile		= sparta::Counter(
					getStatisticSet(),
					"send_other_memtile",
					"Number of operations forwarded to another memory tile",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_received_other_memtile	= sparta::Counter(
					getStatisticSet(),
					"receive_other_memtile",
					"Number of operations received from another memory tile",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_requests_mc				= sparta::Counter(
					getStatisticSet(), 
					"requests_mc", 
					"Number of requests to MC",
					sparta::Counter::COUNT_NORMAL
			);
			
			sparta::Counter count_requests_llc				= sparta::Counter(
					getStatisticSet(), 
					"requests_llc", 
					"Number of requests to the LLC",
					sparta::Counter::COUNT_NORMAL
			);
	};
}
#endif
