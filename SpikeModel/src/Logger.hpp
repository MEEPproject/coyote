#ifndef __LOGGER_HH__
#define __LOGGER_HH__

#include <memory>
#include <fstream>
#include <list>

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
             * \param address The address that has been serviced and is resuming execution
             */
            void logResumeWithAddress(uint64_t timestamp, uint64_t id, uint64_t address);

            /*!
             * \brief Add an L2 Read event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param address The read address
             */
            void logL2Read(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an L2 write event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param address The written address
             */
            void logL2Write(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an L2 writeback event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core (does not fully apply...)
             * \param pc The PC of the instruction related to the event the event (does not fully apply...)
             * \param address The written address
             */
            void logL2WB(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add a Stall event to the trace. This means that a core is waiting on a memory request.
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             */
            void logStall(uint64_t timestamp, uint64_t id, uint64_t pc);

            /*!
             * \brief Add an L2 Miss event to the trace
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param address The missing address
             */
            void logL2Miss(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing a request to a cache bank local to the tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param bank The id of the accessed bank
             * \param address The address in the request
             */
            void logLocalBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address);

            /*!
             * \brief Add an event representing a request to a cache bank on a remote tile
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param tile The destination tile for the request
             * \param The address in the request
             */
            void logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address);

            /*!
             * \brief Add a an event representing a request to a cache bank local to the tile, that is the result of an earlier remote request.
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param tile The tile
             * \param The address in the request
             */
            void logSurrogateBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address);

            /*!
             * \brief Add a an event representing a request to a memory cpu
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param mc The destination memory controller
             * \param The address in the request
             */
            void logMemoryCPURequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address);

            /*!
             * \brief Add an event representing the start of the operation of a memory cpu to service a request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param address The address in the request
             */
            void logMemoryCPUOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing an acknowledgement to memory cpu request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param tile The destination tile for the ack
             * \param address The address in the request
             */
            void logMemoryCPUAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);

            /*!
             * \brief Add a an event representing a request to a memory controller
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param mc The destination memory controller
             * \param The address in the request
             */
            void logMemoryControllerRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address);

            /*!
             * \brief Add an event representing the start of the operation of a memory controller to service a request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param mc The destination memory controller
             * \param address The address in the request
             */
            void logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address);

            /*!
             * \brief Add an event representing an acknowledgement to memory controller request
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param tile The destination tile for the ack
             * \param address The address in the request
             */
            void logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);
    
    
            /*!
             * \brief Add an event representing the submission of a bank to a command 
             * \param timestamp The timestamp for the event
             * \param mc The id of the memory controller for the bank
             * \param pc The PC of the instruction related to the event the event
             * \param bank The bank in the command
             * \param address The address in the request
             * \note Important: this event is not tied to a particular scalar core
             */
            void logMemoryBankCommand(uint64_t timestamp, uint64_t mc, uint64_t pc, uint8_t bank, uint64_t address);

            /*!
             * \brief Add an event representing the reception of a memory controller ack at its destination tile.
             * \param timestamp The logger to use
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param address The address in the request
             */
            void logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing the forwarding of an ack to another tile.
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param tile The destination tile (which was the source tile of an earlier remote request)
             * \param address The address in the request
             */
            void logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);

            /*!
             * \brief Add an event representing the reception of a forwarded ack
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param address The address in the request
             */
            void logTileRecAckForwarded(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            /*!
             * \brief Add an event representing the completion of the service for an L1 miss
             * \param timestamp The timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
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

        private:
            std::shared_ptr<std::ofstream> trace_file_;

            std::shared_ptr<std::list<std::string>> events_of_interest_;

            /*!
             * \brief Implementation: Add a generic event.
             * \param timestamp timestamp for the event
             * \param id The id of the producing core
             * \param pc The PC of the instruction related to the event the event
             * \param ev Event type dependant information
             */
            void log(uint64_t timestamp, uint64_t id,  uint64_t pc, std::string ev);

            /*!
             * \brief Checks if a particular event has to be traced.
             * \return True if the event is in the list of events of interest. If the list is empty, all the events are traced
             */
            bool checkIfEventOfInterest(std::string ev);
    };
}
#endif
