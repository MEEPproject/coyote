#include "Logger.hpp"

namespace spike_model
{

    Logger::Logger()
    {
        trace_file_=std::make_shared<std::ofstream>();
        trace_file_->open("trace");
        *trace_file_ << "timestamp,core,event_type,value" << std::endl;
    }

    void Logger::log(uint64_t timestamp, uint64_t id, std::string ev)
    {
       *trace_file_ << timestamp << "," << id << "," << ev << std::endl; 
    }

    void Logger::logResume(uint64_t timestamp, uint64_t id)
    {
        log(timestamp, id, "resume,0");
    }

    void Logger::logL2Read(uint64_t timestamp, uint64_t id, uint64_t address)
    {
        
        log(timestamp, id, "l2_read,"+std::to_string(address));
    }
    
    void Logger::logL2Write(uint64_t timestamp, uint64_t id, uint64_t address)
    {
        
        log(timestamp, id, "l2_write,"+std::to_string(address));
    }

    void Logger::logStall(uint64_t timestamp, uint64_t id)
    {
        log(timestamp, id, "stall,0");
    }

    void Logger::logL2Miss(uint64_t timestamp, uint64_t id)
    {
        log(timestamp, id, "l2_miss,0");
    }

    void Logger::logLocalBankRequest(uint64_t timestamp, uint64_t id, uint8_t bank)
    {
        log(timestamp, id, "request_to_bank,"+std::to_string(bank));
    }
            
    void Logger::logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint8_t tile)
    {
        log(timestamp, id, "request_to_remote_bank,"+std::to_string(tile));
    }

    void Logger::logMemoryControllerRequest(uint64_t timestamp, uint64_t id, uint8_t mc)
    {
        log(timestamp, id, "request_to_memory_controller,"+std::to_string(mc));
    }

    void Logger::close()
    {
        trace_file_->close();
    }
}
