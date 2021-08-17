
#ifndef __ARBITER_HH__
#define __ARBITER_HH__

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

#include "EventManager.hpp"
#include "NoC/NoCMessage.hpp"
#include "LogCapable.hpp"
#include "NoC/NoC.hpp"

namespace spike_model
{
    class EventManager; //Forward declarations

    class Arbiter : public sparta::Unit, public LogCapable, public spike_model::EventVisitor
    {
        public:

            /*!
             * \class ArbiterParameterSet
             * \brief Parameters for Arbiter model
             */
            class ArbiterParameterSet : public sparta::ParameterSet
            {
            public:
                //! Constructor for ArbiterParameterSet
                ArbiterParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
            };

            //Arbiter::Arbiter(sparta::TreeNode* node, const ArbiterParameterSet *p, int num_inputs) : sparta::Unit(node)
            Arbiter(sparta::TreeNode* node, const ArbiterParameterSet *p);

            //! name of this resource.
            static const char name[];


            void addNoCMsg(std::shared_ptr<NoCMessage> mes, int network_type, int input_unit);

            std::shared_ptr<NoCMessage> getNoCMsg(int network_type, int input_unit);

            bool hasNoCMsg(int network_type, int input_unit);

            bool hasNoCMsgInNetwork(int network_type);

            void setNumInputs(uint16_t cores_per_tile, uint16_t l2_banks_per_tile);

            /*
               First num_cores slots are reserved each for a core in the VAS tile
               Second num_l2_banks slots are reserved one each for a L2 bank in a VAS tile
            */
            int getInputIndex(bool is_core, int id);

            void submit(std::shared_ptr<NoCMessage> msg, bool is_core, int id);

            void submitToNoc();

            sparta::DataOutPort<std::shared_ptr<NoCMessage>> out_port_noc_
            {&unit_port_set_, "out_noc"};

            sparta::UniqueEvent<sparta::SchedulingPhase::PostTick> pending_arbiter_event_
                {&unit_event_set_, "pending_arbiter_event", CREATE_SPARTA_HANDLER(Arbiter, submitToNoc)};

        private:
            uint16_t num_inputs_;
            std::vector<int> rr_cntr;
            uint16_t cores_per_tile_;
            std::vector<std::vector<std::queue<std::shared_ptr<NoCMessage>>>> pending_noc_msgs;
    };
}
#endif
