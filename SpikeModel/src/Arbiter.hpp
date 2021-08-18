
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
#include "map"
#include "CacheBank.hpp"
#include "NoCQueueStatus.hpp"
#include "ArbiterMsg.hpp"

namespace spike_model
{
    class EventManager; //Forward declarations
    class NoC;

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

            Arbiter(sparta::TreeNode* node, const ArbiterParameterSet *p);

            //! name of this resource.
            static const char name[];


            void addNoCMsg(std::shared_ptr<NoCMessage> mes, int network_type, int input_unit);

            std::shared_ptr<NoCMessage> getNoCMsg(int network_type, int input_unit);

            bool hasNoCMsg(int network_type, int input_unit);

            bool hasNoCMsgInNetwork();

            void setNumInputs(uint16_t cores_per_tile, uint16_t l2_banks_per_tile, uint16_t tile_id);

            void setRequestManager(std::shared_ptr<EventManager> r);
            /*
               First num_cores slots are reserved each for a core in the VAS tile
               Second num_l2_banks slots are reserved one each for a L2 bank in a VAS tile
            */
            int getInputIndex(bool is_core, int id);

            void submit(const std::shared_ptr<ArbiterMessage> & msg);

            void submitToNoC(uint64_t current_cycle);

            void setNoC(NoC *noc);

            void addBank(CacheBank *bank);

            CacheBank* getBank(int index);

            std::shared_ptr<NoCMessage> popNoCMsg(int network_type, int input_unit);

            size_t queueSize(int network_type, int input_unit);

            bool isCore(int j);

            bool isInputBelowWatermark(int j);

            std::unique_ptr<sparta::DataInPort<std::shared_ptr<ArbiterMessage>>> in_ports_tile_;

        private:
            uint64_t current_cycle;
            uint16_t num_inputs_;
            int8_t num_outputs_;       //! Number of outputs in the crossbar, currently: equal to the number of NoCs
            std::vector<int> rr_cntr;
            uint16_t cores_per_tile_;
            NoC *noc;
            sparta::TreeNode* node_;
            const size_t q_sz = 10;
            const size_t threshold = 5;
            std::vector<std::vector<std::queue<std::shared_ptr<NoCMessage>>>> pending_noc_msgs;
            std::shared_ptr<EventManager> request_manager_;
            std::vector<CacheBank*> l2_banks;
            uint16_t tile_id_;
    };
}
#endif
