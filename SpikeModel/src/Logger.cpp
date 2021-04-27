#include "Logger.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace spike_model
{

    Logger::Logger()
    {
        trace_file_=std::make_shared<std::ofstream>();
        trace_file_->open("trace");
        *trace_file_ << "timestamp,core,pc,event_type,id,address" << std::endl;
        
        events_of_interest_=std::make_shared<std::list<std::string>>();
    }

    void Logger::log(uint64_t timestamp, uint64_t id, uint64_t pc, std::string ev)
    {
        *trace_file_ << std::dec << timestamp << "," << id << "," << std::hex << pc << "," << ev << std::endl;
    }

    void Logger::logResume(uint64_t timestamp, uint64_t id, uint64_t pc)
    {
        std::string ev="resume";
        if(checkIfEventOfInterest(ev))
        {
            log(timestamp, id, pc, "resume,0,0");
        }
    }

    void Logger::logL2Read(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="l2_read";
        if(checkIfEventOfInterest(ev))
        {
            log(timestamp, id, pc, "l2_read,"+std::to_string(address));
        }
    }

    void Logger::logL2Write(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="l2_write";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev  << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logStall(uint64_t timestamp, uint64_t id, uint64_t pc)
    {
        std::string ev="stall";
        if(checkIfEventOfInterest(ev))
        {
            log(timestamp, id, pc, "stall,0,0");
        }
    }

    void Logger::logL2Miss(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="l2_miss";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logL2WB(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="l2_wb";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logLocalBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        std::string ev="local_request";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(bank) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logSurrogateBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        std::string ev="surrogate_request";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(bank) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address)
    {
        std::string ev="remote_request";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryCPURequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address)
    {
        std::string ev="memory_request";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(mc) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryCPUOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="memory_operation";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << 0 << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryCPUAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::string ev="memory_ack";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryControllerRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address)
    {
        std::string ev="memory_request";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(mc) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="memory_operation";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << 0 << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::string ev="memory_ack";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::stringstream sstream;
        sstream << "ack_received," << 0 << "," << std::hex << address;
        log(timestamp, id, pc, sstream.str());
    }

    void Logger::logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::string ev="ack_forwarded";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logTileRecAckForwarded(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="ack_forward_received";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << 0 << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMissServiced(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="miss_serviced";
        if(checkIfEventOfInterest(ev))
        {
            std::stringstream sstream;
            sstream << ev << "," << 0 << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logKI(uint64_t timestamp, uint64_t id)
    {
        std::string ev="KI";
        if(checkIfEventOfInterest(ev))
        {
           log(timestamp, id, 0, "KI,0,0");
        }
    }

    void Logger::close()
    {
        trace_file_->close();
    }
            
    bool Logger::checkIfEventOfInterest(std::string ev)
    {
        return (events_of_interest_->size()==0 || std::find(events_of_interest_->begin(), events_of_interest_->end(), ev) != events_of_interest_->end());
    }
    
    void Logger::addEventOfInterest(std::string ev)
    {
        events_of_interest_->push_back(ev);
    }
}
