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
            
            void log(uint64_t timestamp, uint64_t id, std::string ev);

            void logResume(uint64_t timestamp, uint64_t id);

            void logL2Read(uint64_t timestamp, uint64_t id, uint64_t address);
            
            void logL2Write(uint64_t timestamp, uint64_t id, uint64_t address);

            void logStall(uint64_t timestamp, uint64_t id);

            void logL2Miss(uint64_t timestamp, uint64_t id);

            void logLocalBankRequest(uint64_t timestamp, uint64_t id, uint8_t bank);

            void logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint8_t tile);

            void logMemoryControllerRequest(uint64_t timestamp, uint64_t id, uint8_t mc);

            void close();

        private:
            std::shared_ptr<std::ofstream> trace_file_;
    };
}
#endif
