#ifndef __MEMORY_CPU_WRAPPER_H__
#define __MEMORY_CPU_WRAPPER_H__

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"
#include "sparta/resources/Pipeline.hpp"
#include "sparta/resources/Buffer.hpp"
#include "sparta/pairs/SpartaKeyPairs.hpp"
#include "sparta/simulation/State.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"

#include "LogCapable.hpp"
#include "NoCMessage.hpp"
#include "Event.hpp"
#include "EventManager.hpp"
#include "MCPURequest.hpp"
#include "MCPUInstruction.hpp"

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

			uint64_t line_size_;
			uint64_t latency_;


			//-- commands coming from the tile
			sparta::UniqueEvent<> issue_mcpu_event_ {
				&unit_event_set_, "issue_mcpu_", CREATE_SPARTA_HANDLER(MemoryCPUWrapper, issueMCPU_)
			};
			std::list<std::shared_ptr<MCPURequest>> mcpu_req;
			void issueMCPU_();


			//-- message handling
			bool idle_noc = true;
			bool idle_mc  = true;
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

			std::shared_ptr<EventManager> request_manager_;
			void receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes);
			void receiveMessage_mc_(const std::shared_ptr<CacheRequest> &mes);
			virtual void handle(std::shared_ptr<spike_model::CacheRequest> r) override;
			virtual void handle(std::shared_ptr<spike_model::MCPURequest> r) override;
            
            /*!
             * \brief Handles an instruction forwarded to the MCPU
             * \param r The instruction to handle
             */
            virtual void handle(std::shared_ptr<spike_model::MCPUInstruction> r) override;


			//-- reporting and logging
			sparta::Counter count_requests_noc_=sparta::Counter(getStatisticSet(), "requests_noc", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_load_=sparta::Counter(getStatisticSet(), "requests_noc_load", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_store_=sparta::Counter(getStatisticSet(), "requests_noc_store", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_requests_mc_=sparta::Counter(getStatisticSet(), "requests_mc", "Number of requests from MC", sparta::Counter::COUNT_NORMAL);

	};
}
#endif
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab: