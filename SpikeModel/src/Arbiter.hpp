
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
#include "L2CacheBank.hpp"
#include "ArbiterMsg.hpp"

namespace spike_model
{
    class EventManager; //Forward declarations
    class NoC;
    class L2CacheBank;

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
                PARAMETER(uint16_t, q_sz, 16, "The size of the arbiter queue for each input unit")
            };

            Arbiter(sparta::TreeNode* node, const ArbiterParameterSet *p);

            //! name of this resource.
            static const char name[];


            void addNoCMsg(std::shared_ptr<NoCMessage> mes, int network_type, int input_unit);

            std::shared_ptr<NoCMessage> getNoCMsg(int network_type, int input_unit);

            bool hasNoCMsg(int network_type, int input_unit);

            bool hasNoCMsgInNetwork();

            void addCacheRequest(std::shared_ptr<CacheRequest> mes, uint16_t bank, int core);

            bool hasCacheRequest(uint16_t bank, int core);

            bool hasCacheRequestInNetwork();

            void setNumInputs(uint16_t cores_per_tile, uint16_t l2_banks_per_tile, uint16_t tile_id);

            /*
               First num_cores slots are reserved each for a core in the VAS tile
               Second num_l2_banks slots are reserved one each for a L2 bank in a VAS tile
            */
            int getInputIndex(bool is_core, int id);

            void submit(const std::shared_ptr<ArbiterMessage> & msg);

            void submitToNoC();

            void submitToL2();

            void setNoC(NoC *noc);

            void addBank(L2CacheBank *bank);

            L2CacheBank* getBank(int index);

            std::shared_ptr<NoCMessage> popNoCMsg(int network_type, int input_unit);

            std::shared_ptr<CacheRequest> popCacheRequest(uint16_t bank, int core);

            bool isCore(int j);

            bool hasArbiterQueueFreeSlot(uint16_t tile_id, uint16_t core_id);

            bool hasNoCQueueFreeSlot(uint16_t core_id);

            bool hasL1L2QueueFreeSlot(uint16_t core_id);

            bool hasL2NoCQueueFreeSlot(uint16_t bank_id);

            size_t NoCQueueSize(int network_type, int input_unit);

            size_t L2QueueSize(uint16_t bank, uint16_t core);

            std::unique_ptr<sparta::DataInPort<std::shared_ptr<ArbiterMessage>>> in_ports_tile_;

        private:
            uint16_t num_inputs_;
            int8_t num_outputs_;       //! Number of outputs in the crossbar, currently: equal to the number of NoCs
            std::vector<int> rr_cntr_noc_;
            std::vector<int> rr_cntr_cache_req_;
            uint16_t cores_per_tile_;
            uint16_t num_l2_banks_;
            NoC *noc;
            std::vector<std::vector<std::queue<std::shared_ptr<NoCMessage>>>> pending_noc_msgs_;
            std::vector<std::vector<std::queue<std::shared_ptr<CacheRequest>>>> pending_l2_msgs_;
            std::vector<L2CacheBank*> l2_banks;
            size_t q_sz;
            uint16_t tile_;
            sparta::Counter count_cache_requests_ = sparta::Counter
            (
                getStatisticSet(),                  // parent
                "num_cache_requests",               // name
                "Number of Cache Requests",         // description
                sparta::Counter::COUNT_NORMAL       // behavior
            );
            sparta::Counter count_noc_messages_ = sparta::Counter
            (
                getStatisticSet(),                  // parent
                "num_noc_messages",                 // name
                "Number of NoC Messages",           // description
                sparta::Counter::COUNT_NORMAL       // behavior
            );
    };
}
#endif
