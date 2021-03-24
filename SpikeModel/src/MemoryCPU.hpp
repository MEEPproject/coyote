
#ifndef __MEMORY_CPU_H__
#define __MEMORY_CPU_H__

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

#include <memory>
#include <queue>

#include "LogCapable.hpp"
#include "NoCMessage.hpp"
#include "RequestManagerIF.hpp"
#include "Request.hpp"

namespace spike_model {
	class MemoryCPU : public sparta::Unit, public LogCapable {
		public:
			/*!
			 * \class MemoryCPUParameterSet
			 * \brief Parameters for MemoryCPU model
			 */
			class MemoryCPUParameterSet : public sparta::ParameterSet {
				public:
					//! Constructor for MemoryCPUParameterSet
					MemoryCPUParameterSet(sparta::TreeNode* n):sparta::ParameterSet(n) { }
					PARAMETER(uint64_t, latency, 100, "The latency in the memory controller")
			};

			/*!
			 * \brief Constructor for MemoryCPU
			 * \note  node parameter is the node that represent the MemoryCPU and
			 *        p is the MemoryCPU parameter set
			 */
			MemoryCPU(sparta::TreeNode* node, const MemoryCPUParameterSet* p);

			~MemoryCPU() {
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
			void setRequestManager(std::shared_ptr<RequestManagerIF> r);

		private:

			sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc_{
				&unit_port_set_, "out_noc"
			};

			sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc_ {
				&unit_port_set_, "in_noc"
			};

			sparta::DataOutPort<std::shared_ptr<Request>> out_port_mc_{
				&unit_port_set_, "out_mc"
			};

			sparta::DataInPort<std::shared_ptr<Request>> in_port_mc_{
				&unit_port_set_, "in_mc"
			};

			//sparta::UniqueEvent<> controller_cycle_event_{
			//	&unit_event_set_, "controller_cycle_", CREATE_SPARTA_HANDLER(MemoryCPU, controllerCycle_)
			//};

			uint64_t latency_;

			bool idle_noc = true;
			bool idle_mc  = true;
			std::shared_ptr<Request> msg_from_noc;
			std::shared_ptr<Request> msg_from_mc;

			std::shared_ptr<RequestManagerIF> request_manager_;

			sparta::Counter count_requests_noc_=sparta::Counter(getStatisticSet(), "requests_noc", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_load_=sparta::Counter(getStatisticSet(), "requests_noc_load", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_store_=sparta::Counter(getStatisticSet(), "requests_noc_store", "Number of requests from NoC", sparta::Counter::COUNT_NORMAL);
			sparta::Counter count_requests_mc_=sparta::Counter(getStatisticSet(), "requests_mc", "Number of requests from MC", sparta::Counter::COUNT_NORMAL);

			void receiveMessage_noc_(const std::shared_ptr<NoCMessage> &mes);
			void receiveMessage_mc_(const std::shared_ptr<Request> &mes);

			void controllerCycle_();
	};
}
#endif
