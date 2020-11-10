#include "Logger.hpp"

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
       *trace_file_ << timestamp << "," << id << "," << pc << "," << ev << std::endl; 
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
        
        log(timestamp, id, pc, "l2_write,"+std::to_string(address));
    }

    void Logger::logStall(uint64_t timestamp, uint64_t id, uint64_t pc)
    {
        log(timestamp, id, pc, "stall,0,0");
    }

    void Logger::logL2Miss(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        log(timestamp, id, pc, "l2_miss,0,"+std::to_string(address));
    }

    void Logger::logLocalBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        log(timestamp, id, pc, "local_request,"+std::to_string(bank)+","+std::to_string(address));
    }
    
    void Logger::logSurrogateBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        log(timestamp, id, pc, "surrogate_request,"+std::to_string(bank)+","+std::to_string(address));
    }
            
    void Logger::logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address)
    {
        log(timestamp, id, pc, "remote_request,"+std::to_string(tile)+","+std::to_string(address));
    }

    void Logger::logMemoryControllerRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address)
    {
        log(timestamp, id, pc, "memory_request,"+std::to_string(mc)+","+std::to_string(address));
    }
    
    void Logger::logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        log(timestamp, id, pc, "memory_operation,0,"+std::to_string(address));
    }
    
    void Logger::logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        log(timestamp, id, pc, "memory_ack,"+std::to_string(tile)+","+std::to_string(address));
    }
    
    void Logger::logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        log(timestamp, id, pc, "ack_received,0,"+std::to_string(address));
    }
    
    void Logger::logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        log(timestamp, id, pc, "ack_forwarded,"+std::to_string(tile)+","+std::to_string(address));
    }
    
    void Logger::logTileRecAckForwarded(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        log(timestamp, id, pc, "ack_forward_received,0,"+std::to_string(address));
    }
            
    void Logger::logMissServiced(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        log(timestamp, id, pc, "miss_serviced,0,"+std::to_string(address));
    }

    void Logger::close()
    {
        trace_file_->close();
    }
}
