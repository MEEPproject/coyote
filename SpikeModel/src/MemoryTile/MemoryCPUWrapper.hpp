#ifndef __MEMORY_CPU_WRAPPER_H__
#define __MEMORY_CPU_WRAPPER_H__

#include <queue>

//#include "sparta/ports/PortSet.hpp"
//#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
//#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
//#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
//#include "sparta/collection/Collectable.hpp"
//#include "sparta/events/StartupEvent.hpp"
//#include "sparta/resources/Pipeline.hpp"
//#include "sparta/resources/Buffer.hpp"
//#include "sparta/pairs/SpartaKeyPairs.hpp"
//#include "sparta/simulation/State.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"

#include "LogCapable.hpp"
#include "NoC/NoCMessage.hpp"
#include "Event.hpp"
#include "EventManager.hpp"
#include "MCPUSetVVL.hpp"
#include "MCPUInstruction.hpp"
#include "CacheRequest.hpp"
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
					PARAMETER(uint64_t, line_size, 128, "The cache line size")
					PARAMETER(uint64_t, latency, 100, "The latency in the memory CPU wrapper")
			};

			/*!
			 * \brief Constructor for MemoryCPUWrapper
			 * \note  node parameter is the node that represent the MemoryCPUWrapper and
			 *        p is the MemoryCPUWrapper parameter set
			 */
			MemoryCPUWrapper(sparta::TreeNode* node, const MemoryCPUWrapperParameterSet* p);

			~MemoryCPUWrapper() {
				debug_logger_ << getContainer()->getLocation()
					<< ": "
					<< std::endl;
			}

			//! name of this resource.
			static const char name[];

			void notifyCompletion_();

			/*!
			 * \brief Sets the request manager for the tile
			 */
			void setRequestManager(std::shared_ptr<EventManager> r);


		private:

			uint32_t line_size_;
			uint32_t vvl_;
			uint64_t latency_;
			uint32_t instructionID_counter; // ID issued to incoming mcpu instructions. increments with every new instruction
			struct transaction {
                std::shared_ptr<MCPUInstruction> mcpu_instruction;
				uint32_t counter_cacheRequests;
				uint32_t counter_scratchpadRequests;
				uint32_t number_of_elements_per_response;
			};
			std::unordered_map<std::uint32_t, transaction> transaction_table;
            
			//-- commands coming from the tile
			//sparta::UniqueEvent<> issue_mcpu_event_ {
			//	&unit_event_set_, "issue_mcpu_", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, issueMCPU_)
			//};
			//std::list<std::shared_ptr<MCPUSetVVL>> mcpu_req;
			//void issueMCPU_();


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
		    sparta::UniqueEvent<sparta::SchedulingPhase::PostTick> controller_cycle_event_mem_req {
			   &unit_event_set_, "controller_cycle_mem_requests", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, controllerCycle_mem_requests)
			}; 
			Bus<std::shared_ptr<CacheRequest>> sched_mem_req;

			//-- Bus for incoming transactions
			sparta::UniqueEvent<sparta::SchedulingPhase::PostTick> controller_cycle_event_incoming_transaction {
					&unit_event_set_, "controller_cycle_incoming_transaction", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, controllerCycle_incoming_transaction)
			};
			Bus<std::shared_ptr<MCPUInstruction>> sched_incoming;
			
			//-- Bus for outgoing transactions
			sparta::UniqueEvent<sparta::SchedulingPhase::PostTick> controller_cycle_event_outgoing_transaction {
					&unit_event_set_, "controller_cycle_outgoing_transaction", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, controllerCycle_outgoing_transaction)
			};
			Bus<std::shared_ptr<ScratchpadRequest>> sched_outgoing;


			std::shared_ptr<EventManager> request_manager_;
			
			
			void receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes);
			void receiveMessage_mc_(const std::shared_ptr<CacheRequest> &mes);
			
			/*!
			 * \brief Handles an instruction forwarded to the MCPU
			 * \param r The instruction to handle
			 */
			virtual void handle(std::shared_ptr<spike_model::CacheRequest> r) override;
			virtual void handle(std::shared_ptr<spike_model::MCPUSetVVL> r) override;
			virtual void handle(std::shared_ptr<spike_model::MCPUInstruction> r) override;


			void controllerCycle_incoming_transaction(); // for incoming MCPUInstructions
			void controllerCycle_outgoing_transaction();
			void controllerCycle_mem_requests(); // 
			void schedule_outgoing_mem_ops(); // Outgoing transaction queue
			void memOp_unit(std::shared_ptr<MCPUInstruction> instr);
			void memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr);
			void memOp_orderedIndex(std::shared_ptr<MCPUInstruction> instr);
			void memOp_unorderedIndex(std::shared_ptr<MCPUInstruction> instr);
			

			//-- reporting and logging
			sparta::Counter count_requests_noc_=sparta::Counter(getStatisticSet(), "requests_noc", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_load_=sparta::Counter(getStatisticSet(), "requests_noc_load", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_store_=sparta::Counter(getStatisticSet(), "requests_noc_store", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_requests_mc_=sparta::Counter(getStatisticSet(), "requests_mc", "Number of requests from MC", sparta::Counter::COUNT_NORMAL);
			
	};
}
#endif
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab: