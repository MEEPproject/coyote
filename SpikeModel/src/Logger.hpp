#ifndef __LOGGER_HH__
#define __LOGGER_HH__

#include <memory>
#include <fstream>
#include <list>
#include <limits>

namespace spike_model
{
    class Logger
    {
        /*!
         * \class spike_model::Logger
         * \brief The Logger class is the writer for the trace of the simulation
         */
        public:

            Logger();

            /*!
             * \brief Add a resume event to the trace. This means that a core is no longer stalled
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             */
            void logResume(uint64_t timestamp, uint64_t id);
            
            /*!
             * \brief Add a resume event to the trace. This means that a core is no longer stalled
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param mc The id of the memory controller that serviced the request that resumes the core
             */
            void logResumeWithMC(uint64_t timestamp, uint64_t id, uint64_t mc);

            /*!
             * \brief Add a resume event to the trace. This means that a core is no longer stalled
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param mem_bank The id of the memory bank that serviced the request that resumes the core
             */
            void logResumeWithMemBank(uint64_t timestamp, uint64_t id, uint64_t mem_bank);
            
            /*!
             * \brief Add a resume event to the trace. This means that a core is no longer stalled
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param mem_bank The id of the cache bank that serviced the request that resumes the core
             */
            void logResumeWithCacheBank(uint64_t timestamp, uint64_t id, uint64_t cache_bank);

            /*!
             * \brief Add a resume event to the trace. This means that a core is no longer stalled
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param tile_id The id of the tile that serviced the request that resumes the core
             */
            void logResumeWithTile(uint64_t timestamp, uint64_t id, uint64_t tile_id);

            /*!
             * \brief Add a resume event to the trace. This means that a core is no longer stalled
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param address The address that has been serviced and is resuming execution
             */
            void logResumeWithAddress(uint64_t timestamp, uint64_t id, uint64_t address);

            /*!
             * \brief Add an L2 Read event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The read address
             * \param size The size to read
             */
            void logL2Read(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size);

            /*!
             * \brief Add an L2 write event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The written address
             * \param size The size to write
             */
            void logL2Write(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size);
            
            /*!
             * \brief Add an LLC Read event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The read address
             * \param size The size to read
             */
            void logLLCRead(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size);

            /*!
             * \brief Add an LLC write event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The written address
             * \param size The size to write
             */
            void logLLCWrite(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size);

            /*!
             * \brief Add an L2 writeback event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core (does not fully apply...)
             * \param pc The PC of the instruction related to the event (does not fully apply...)
             * \param address The written address
             * \param size The size to write back
             */
            void logL2WB(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size);

            /*!
             * \brief Add a Stall event to the trace. This means that a core is waiting on a memory request.
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             */
            void logStall(uint64_t timestamp, uint64_t id, uint64_t pc);

            /*!
             * \brief Add an L2 Miss event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The missing address
             */
            void logL2Miss(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing a request to a cache bank local to the tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param bank The id of the accessed bank
             * \param address The address in the request
             */
            void logLocalBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address);

            /*!
             * \brief Add an event representing a request to a cache bank on a remote tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param tile The destination tile for the request
             * \param The address in the request
             */
            void logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address);

            /*!
             * \brief Add a an event representing a request to a cache bank local to the tile, that is the result of an earlier remote request.
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param tile The tile
             * \param The address in the request
             */
            void logSurrogateBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address);

            /*!
             * \brief Add a an event representing a request to a memory cpu
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param mc The destination memory controller
             * \param The address in the request
             */
            void logMemoryCPURequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address);

            /*!
             * \brief Add an event representing the start of the operation of a memory cpu to service a request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The address in the request
             */
            void logMemoryCPUOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing an acknowledgement to memory cpu request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param tile The destination tile for the ack
             * \param address The address in the request
             */
            void logMemoryCPUAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);

            /*!
             * \brief Add a an event representing a read request to a memory controller
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param size The size of the read
             * \param The address in the request
             */
            void logMemoryControllerRead(uint64_t timestamp, uint64_t id, uint64_t pc, uint32_t size, uint64_t address);

            /*!
             * \brief Add a an event representing a write request to a memory controller
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param size The size of the write
             * \param The address in the request
             * \param The size to write
             */
            void logMemoryControllerWrite(uint64_t timestamp, uint64_t id, uint64_t pc, uint32_t size, uint64_t address);

            /*!
             * \brief Add an event representing the start of the operation of a memory controller to service a request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param mc The destination memory controller
             * \param address The address in the request
             */
            void logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address);

            /*!
             * \brief Add an event representing an acknowledgement to memory controller request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param tile The destination tile for the ack
             * \param address The address in the request
             */
            void logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);
    
    
            /*!
             * \brief Add an event representing the submission of a bank to a command 
             * \param timestamp The timestamp for the event
             * \param mc The id of the memory controller for the bank
             * \param pc The PC of the instruction related to the event
             * \param bank The bank in the command
             * \param address The address in the request
             * \note Important: this event is not tied to a particular scalar core
             */
            void logMemoryBankCommand(uint64_t timestamp, uint64_t mc, uint64_t pc, uint8_t bank, uint64_t address);

            /*!
             * \brief Add an event representing the reception of a memory controller ack at its destination tile.
             * \param timestamp The logger to use
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The address in the request
             */
            void logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing the forwarding of an ack to another tile.
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param tile The destination tile (which was the source tile of an earlier remote request)
             * \param address The address in the request
             */
            void logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);

            /*!
             * \brief Add an event representing the reception of a forwarded ack
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The address in the request
             */
            void logTileRecAckForwarded(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing the completion of the service for an L1 miss
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param address The address in the request
             * \param timestamp
             */
            void logMissServiced(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event informing that 1000 instructions have been simulated in a core
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             */
            void logKI(uint64_t timestamp, uint64_t id);

            //-- Memory Tile Logs
            /*!
             * \brief Occupancy of the outgoing queue connected to the NoC router
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param value The current level of occupancy
             */
            void logMemTileOccupancyOutNoC(uint64_t timestamp, uint16_t id, uint32_t value);

            /*!
             * \brief Occupancy of the queue connecting the memory tile to the memory controller
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param value The current level of occupancy
             */
            void logMemTileOccupancyMC(uint64_t timestamp, uint16_t id, uint32_t value);
            
            /*!
             * \brief Setting of VVL
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param core_id The core ID the VVL setting belongs to
             * \param value The current level of occupancy
             */
            void logMemTileVVL(uint64_t timestamp, uint16_t id, uint64_t core_id, uint32_t value);
            
            /*!
             * \brief Vector operation received by the MemTile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param tile The source tile
             */
            void logMemTileVecOpRecv(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t address);
            
            /*!
             * \brief Vector operation returned to the VAS Tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param tile The destination tile
             */
            void logMemTileVecOpSent(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t address);
            
            /*!
             * \brief Scalar operation received by the MemTile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param tile The source tile
             */
            void logMemTileScaOpRecv(uint64_t timestamp, uint16_t id, uint64_t tile, uint64_t address);
            
            /*!
             * \brief Scalar operation returned to the VAS Tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param tile The destination tile
             */
            void logMemTileScaOpSent(uint64_t timestamp, uint16_t id, uint64_t tile, uint64_t address);
            
            /*!
             * \brief Scratchpad operation received by the MemTile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param tile The source tile
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileSPOpRecv(uint64_t timestamp, uint16_t id, uint64_t tile, uint64_t parent_address);
            
            /*!
             * \brief Scratchpad operation returned to the VAS Tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param tile The destination tile
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileSPOpSent(uint64_t timestamp, uint16_t id, uint64_t tile, uint64_t parent_address);

            /*!
             * \brief Memory Tile Request received by another Memory Tile
             * \param timestamp The timestamp for the event
             * \param id The id of the Memory Tile that received the request
             * \param src_id The ID of the Memory Tile the produced the request
             */
            void logMemTileMTOpRecv(uint64_t timestamp, uint16_t id, uint16_t src_id, uint64_t address);
            
            /*!
             * \brief Memory Tile Request sent to another Memory Tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param dest_id The destination memory tile
             */
            void logMemTileMTOpSent(uint64_t timestamp, uint16_t id, uint16_t dest_id, uint64_t address);
            
            /*!
             * \brief A memory request is returned from the memory controller
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param address The address being contacted
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileMCRecv(uint64_t timestamp, uint16_t id, uint64_t address);
            void logMemTileMCRecv(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address);
            
            /*!
             * \brief A memory request is forwarded to the memory controller
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param address The address being contacted
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileMCSent(uint64_t timestamp, uint16_t id, uint64_t address);
            void logMemTileMCSent(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address);
            
            /*!
             * \brief A memory request is forwarded to the LLC
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param address The address being contacted
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileLLCRecv(uint64_t timestamp, uint16_t id, uint64_t address);
            void logMemTileLLCRecv(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address);

            /*!
             * \brief A memory request is forwarded to the LLC
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param address The address being contacted
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileLLCSent(uint64_t timestamp, uint16_t id, uint64_t address);
            void logMemTileLLCSent(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address);

            /*!
             * \brief A memory request is forwarded from the LLC to the MC
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param address The address being contacted
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileLLC2MC(uint64_t timestamp, uint16_t id, uint64_t address);
            void logMemTileLLC2MC(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address);

            /*!
             * \brief A memory request is returned from the MC to the LLC
             * \param timestamp The timestamp for the event
             * \param id The id of the producing memory tile
             * \param address The address being contacted
             * \param parent_address The parent address that this instruction for the SP belongs to
             */
            void logMemTileMC2LLC(uint64_t timestamp, uint16_t id, uint64_t address);
            void logMemTileMC2LLC(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address);

            /*!
             * \brief An NoC message is forwarded
             * \param timestamp The timestamp for the event
             * \param id The id of the memory tile
             * \param destAddress The destination address of the NoC message
             */
            void logMemTileNoCSent(uint64_t timestamp, uint16_t id, uint16_t destAddress, uint64_t address);
            
            /*!
             * \brief An NoC message has been received
             * \param timestamp The timestamp for the event
             * \param id The id of the memory tile
             * \param srcAddress Where the message came from.
             */
            void logMemTileNoCRecv(uint64_t timestamp, uint16_t id, uint16_t srcAddress, uint64_t address);
            /*!
             * \brief Close the trace
             */
            void close();

            /*!
             * \brief Add an event that should be traced
             * \param ev An event that should be traced
             * \note If no event is added, every event is traced
             */
            void addEventOfInterest(std::string ev);

            /*!
             * \brief Add an L2 writeback event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core (does not fully apply...)
             * \param pc The PC of the instruction related to the event (does not fully apply...)
             * \param address The accessed address
             * \param time_since_eviction The number of cycles that have passed since the line was evicted
             */
            void logMissOnEvicted(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint64_t time_since_eviction);

            
            /*!
             * \brief Add an NoCMessage Source event to the trace
             * \param timestamp The timestamp for the event
             * \parram core_id The id of the core involved in this event
             * \param src_id The id of the source of the message
             * \param pc The PC of the instruction related to the event (does not fully apply...)
             */
            void logNoCMessageSource(uint64_t timestamp, uint64_t core_id, uint64_t src_id, uint64_t pc);

            /*!
             * \brief Add a cummulated NoCMessage Destination event to the trace
             * \param timestamp The timestamp for the event
             * \parram core_id The id of the core involved in this event
             * \param dst_id The id of the source of the message
             * \param pc The PC of the instruction related to the event (does not fully apply...)
             */
            void logNoCMessageDestination(uint64_t timestamp, uint64_t core_id, uint64_t dst_id, uint64_t pc);
            
            /*!
             * \brief Add a cummulated NoCMessage Destination event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the source of the message
             * \param pc The PC of the instruction related to the event (does not fully apply...)
             * \param num_packets The number of packets that have been enqueued since the last event
             */
            void logNoCMessageDestinationCummulated(uint64_t timestamp, uint64_t dst_id, uint64_t pc, uint64_t num_packets);

            /*!
             * \brief Set the time bounds within which events are traced
             * \param lower The lower bound
             * \param upper The upper bound
             */
            void setTimeBounds(uint64_t lower, uint64_t upper);
            
            std::shared_ptr<std::ofstream> getFile()
            {
                return trace_file_;
            }

        private:
            std::shared_ptr<std::ofstream> trace_file_;

            std::shared_ptr<std::list<std::string>> events_of_interest_;

            std::shared_ptr<uint64_t> lower_bound_=std::make_shared<uint64_t> (0);
            std::shared_ptr<uint64_t> upper_bound_=std::make_shared<uint64_t> (std::numeric_limits<uint64_t>::max());

            /*!
             * \brief Implementation: Add a generic event.
             * \param timestamp timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event
             * \param ev Event type dependant information
             */
            void log(uint64_t timestamp, uint64_t id,  uint64_t pc, std::string ev);

            /*!
             * \brief Checks if a particular event has to be traced.
             * \return True if the event is in the list of events of interest. If the list is empty, all the events are traced
             */
            bool checkIfEventOfInterest(std::string ev);
            
            /*!
             * \brief Check if a particular timestamp has to be traced
             * \param t The timestamp to check
             */
            bool checkBounds(uint64_t t);
    };
}
#endif
