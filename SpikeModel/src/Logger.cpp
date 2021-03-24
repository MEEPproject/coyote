#include "Logger.hpp"
#include <sstream>

namespace spike_model
{

    Logger::Logger()
    {
        trace_file_=std::make_shared<std::ofstream>();
        trace_file_->open("trace");
        *trace_file_ << "timestamp,core,pc,event_type,id,address" << std::endl;
    }

    void Logger::log(uint64_t timestamp, uint64_t id, uint64_t pc, std::string ev)
    {
       *trace_file_ << std::dec << timestamp << "," << id << "," << std::hex << pc << "," << ev << std::endl;
    }

    void Logger::logResume(uint64_t timestamp, uint64_t id, uint64_t pc)
    {
        log(timestamp, id, pc, "resume,0,0");
    }

    void Logger::logL2Read(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {

        log(timestamp, id, pc, "l2_read,"+std::to_string(address));
    }

    void Logger::logL2Write(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "l2_write," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logStall(uint64_t timestamp, uint64_t id, uint64_t pc)
    {
        log(timestamp, id, pc, "stall,0,0");
    }

    void Logger::logL2Miss(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "l2_miss,0," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logL2WB(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "l2_wb,0," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logLocalBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "local_request," << unsigned(bank) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logSurrogateBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "surrogate_request," << unsigned(bank) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "remote_request," << unsigned(tile) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logMemoryCPURequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "memory_request," << unsigned(mc) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logMemoryCPUOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "memory_operation," << 0 << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logMemoryCPUAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "memory_ack," << unsigned(tile) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logMemoryControllerRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "memory_request," << unsigned(mc) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "memory_operation," << 0 << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "memory_ack," << unsigned(tile) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "ack_received," << 0 << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "ack_forwarded," << unsigned(tile) << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logTileRecAckForwarded(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "ack_forward_received," << 0 << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logMissServiced(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "miss_serviced," << 0 << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logKI(uint64_t timestamp, uint64_t id)
    {

        log(timestamp, id, 0, "KI,0,0");
    }

    void Logger::close()
    {
        trace_file_->close();
    }
}
