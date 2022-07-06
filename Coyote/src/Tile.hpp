
#ifndef __TILE_HH__
#define __TILE_HH__

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

#include "FullSystemSimulationEventManager.hpp"
#include "NoC/NoCMessage.hpp"
#include "LogCapable.hpp"
#include "AddressMappingPolicy.hpp"
#include "AccessDirector.hpp"
#include "MemoryTile/MCPUSetVVL.hpp"
#include "MemoryTile/MCPUInstruction.hpp"
#include "CacheRequest.hpp"
#include "InsnLatencyEvent.hpp"
#include "Arbiter.hpp"
#include "ArbiterMsg.hpp"
#include "SimulationEntryPoint.hpp"

namespace coyote
{
    class FullSystemSimulationEventManager; //Forward declarations
    class NoCMessage;
    class Arbiter;

    class Tile : public sparta::Unit, public LogCapable, public EventVisitor, public SimulationEntryPoint
    {
        using coyote::EventVisitor::handle; //This prevents the compiler from warning on overloading 
        friend class AccessDirector;
        friend class FullSystemSimulationEventManager; //Friendship is not inherited. This is to access private method putEvent from SimulationEntryPoint.

        /*!
         * \class coyote::Tile
         * \brief A representation of a tile in the modelled architecture.
         *
         * Tiles have a chunk of L2 and access to the network on chip, to communicate with
         * other tiles and the memory. They are the main access and exit point of thr architecture
         * modelled using Sparta.
         *
         */
            
        public:
 
            /*!
             * \class TileParameterSet
             * \brief Parameters for Tile model
             */
            class TileParameterSet : public sparta::ParameterSet
            {
            public:
                //! Constructor for TileParameterSet
                TileParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
                PARAMETER(uint16_t, num_l2_banks, 1, "The number of l2 cache banks in the tile")
                PARAMETER(uint64_t, latency, 1, "The number of cycles to get to a local cache bank")
                PARAMETER(std::string, l2_sharing_mode, "tile_private", "How the cache will be shared among the tiles")
                PARAMETER(std::string, bank_policy, "set_interleaving", "The data mapping policy for banks")
                PARAMETER(std::string, scratchpad_policy, "core_to_bank", "The data mapping policy for the scratchpad")
                PARAMETER(std::string, tile_policy, "set_interleaving", "The data mapping policy for tiles")
            };

            /*!
             * \brief Constructor for Tile
             * \note  node parameter is the node that represent the Tile and
             *        p is the Tile parameter set
             */
            Tile(sparta::TreeNode* node, const TileParameterSet* p);

            ~Tile() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            //! name of this resource.
            static const char name[];

            /*!
             * \brief Set the information on the l2 cache banks associated to this tile
             * \param size The size of rach bank
             * \param assoc The associativity of the cache
             * \param line_size The size of the lines of cache
             */
            void setL2BankInfo(uint64_t size, uint64_t assoc, uint64_t line_size);

            void insnLatencyCallback(const std::shared_ptr<InsnLatencyEvent>& r);

            Arbiter* getArbiter();

            void setArbiter(Arbiter *arbiter);
            /*!
             * \brief Set the request manager for the tile
             * \param r The request manager
             */
            void setRequestManager(std::shared_ptr<FullSystemSimulationEventManager> r);

            /*!
             * \brief Set the id for the tile
             * \param id The id
             */
            void setId(uint16_t id);

            uint16_t getId();

            void setCoresPerTile(uint16_t num_cores)
            {
                num_cores_ = num_cores;
            }

            uint16_t getCoresPerTile()
            {
                return num_cores_;
            }

            void setNumTiles(uint16_t num_tiles)
            {
                num_tiles_ = num_tiles;
            }

            uint16_t getNumTiles()
            {
                return num_tiles_;
            }
            
            /*!
             * \brief Notify the completion of the service for an L2 request
             * \param req The request
             */
            void notifyAck_(const std::shared_ptr<Request> & req);
            
             /*!
             * \brief Handles a cache request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::CacheRequest> r) override;
             
             /*!
             * \brief Handles a scratchpad request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::ScratchpadRequest> r) override;
            
            /*!
             * \brief Handles a MCPU request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::MCPUSetVVL> r) override;
            
            /*!
             * \brief Handles a InstLatency event
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::InsnLatencyEvent> r) override;

            /*!
             * \brief Handles an instruction forwarded to the MCPU
             * \param r The instruction to handle
             */
            virtual void handle(std::shared_ptr<coyote::MCPUInstruction> r) override;
            
            /*!
             * \brief Set the information on the memory hierarchy
             * \param l2_tile_size The total size of the L2 that belongs to each Tile
             * \param assoc The associativity
             * \param line_size The line size
             * \param banks_per_tile The number of banks per Tile
             * \param num_tiles The number of tiles in the system
             * \param num_mcs The number of memory controllers
             * \param mc_shift The number of bits to shift to get the memory controller that handles an address
             * \param mc_mask The mask for the AND to extract the memory controller id
             * \param num_cores The number of cores
             * \param corr_mcpu The MCPU that will handle requests fromthis tile
             */
            void setMemoryInfo(uint64_t l2_tile_size, uint64_t assoc, uint64_t line_size, uint64_t banks_per_tile, uint16_t num_tiles, 
                                uint64_t num_mcs, uint64_t mc_shift, uint64_t mc_mask, uint16_t num_cores, uint16_t corr_mcpu);

            uint16_t getL2Banks();
            std::shared_ptr<FullSystemSimulationEventManager> getRequestManager();
            
        protected:
            /*!
             * \brief Enqueue an event to the tile
             * \param The event to submt 
             */
            virtual void putEvent(const std::shared_ptr<Event> & ) override;

        private:
            uint16_t id_;

            uint16_t num_l2_banks_;
            uint64_t latency_;
            NoCMessageType msgType;
            bool handleNocMsg;
            std::string l2_sharing_mode_;
            std::string bank_policy_;
            std::string scratchpad_policy_;
            std::string tile_policy_;
 
            std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<Request>>>> in_ports_l2_acks_;
            std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<CacheRequest>>>> in_ports_l2_reqs_;
            sparta::DataInPort<std::shared_ptr<NoCMessage>> in_port_noc_
            {&unit_port_set_, "in_noc"};

            std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<CacheRequest>>>> out_ports_l2_acks_;
            std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<Request>>>> out_ports_l2_reqs_;
            sparta::DataOutPort<std::shared_ptr<ArbiterMessage>> out_port_arbiter_
            {&unit_port_set_, "out_arbiter"};

            sparta::PayloadEvent<std::shared_ptr<InsnLatencyEvent>, sparta::SchedulingPhase::PostTick> insn_latency_event_ {&unit_event_set_, "insn_latency_event_", CREATE_SPARTA_HANDLER_WITH_DATA(Tile, insnLatencyCallback, std::shared_ptr<InsnLatencyEvent>)};

            sparta::Counter count_local_requests_=sparta::Counter(getStatisticSet(), "requests_from_local_cores", "Number of cache requests from local cores", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_remote_requests_=sparta::Counter(getStatisticSet(), "requests_from_remote_cores", "Number of cache requests from remote cores", sparta::Counter::COUNT_NORMAL);

            uint64_t cntr;
            uint64_t l2_bank_size_kbs;
            uint64_t l2_assoc;
            uint64_t l2_line_size;

            uint8_t block_offset_bits;
            uint8_t bank_bits;
            uint8_t set_bits; 
            uint8_t tag_bits;
            uint16_t num_cores_;
            uint16_t num_tiles_;
            uint16_t corresponding_mcpu;
    
            Arbiter *arbiter;

            std::shared_ptr<FullSystemSimulationEventManager> request_manager_;

            /*!
             * \brief Send a request to a memory controller
             * \param req The request
             */
            void issueMemoryControllerRequestFromL2_(const std::shared_ptr<CacheRequest> & req);

            void issueMemoryControllerRequest_(const std::shared_ptr<CacheRequest> & req, bool);
            
            /*!
             * \brief Sends an L2 request to a bank in a different tile
             * \param req The request
             */
            void issueRemoteRequest_(const std::shared_ptr<CacheRequest> & req, uint64_t lapse);
            
            /*!
             * \brief Sends an L2 request to a bank in the current tile
             * \param req The request
             */
            void issueLocalRequest_(const std::shared_ptr<Request> & req, uint64_t lapse);
            
            /*!
             * \brief Sends an ack to a cache bank of the current tile (a prior remote L2 request or memory request has been serviced)
             * \param req The request
             */
            void issueBankAck_(const std::shared_ptr<CacheRequest> & req);
            
            /*!
             * \brief Handles a message from the NoC
             * \param req The request
             */
            void handleNoCMessage_(const std::shared_ptr<NoCMessage> & mes);

            AccessDirector * access_director;
            sparta::TreeNode *node_;

    };
}
#endif
