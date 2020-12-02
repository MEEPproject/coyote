#ifndef __LOGGER_HH__
#define __LOGGER_HH__

#include <memory>
#include <fstream>

namespace spike_model
{
    class Logger
    {
        public:
            Logger();
            
            void log(uint64_t timestamp, uint64_t id,  uint64_t pc, std::string ev);

            void logResume(uint64_t timestamp, uint64_t id, uint64_t pc);

            void logL2Read(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);
            
            void logL2Write(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            void logStall(uint64_t timestamp, uint64_t id, uint64_t pc);

            void logL2Miss(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            void logLocalBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address);

            void logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address);
            
            void logSurrogateBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address);

            void logMemoryControllerRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address);

            void logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);
            
            void logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);
            
            void logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);
            
            void logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address);
    
            void logTileRecAckForwarded(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);

            void logMissServiced(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address);
            
            void logKI(uint64_t timestamp, uint64_t id);

            void close();

        private:
            std::shared_ptr<std::ofstream> trace_file_;
    };
}
#endif
