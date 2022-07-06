#include "Logger.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>
#include "StallReason.hpp"
#include "utils.hpp"

namespace coyote
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
            
    void Logger::logRaw(uint64_t timestamp, char * text)
    {
        *trace_file_ << std::dec << timestamp << ", " << text << std::endl;
    }

    void Logger::logResumeWithMC(uint64_t timestamp, uint64_t id, uint64_t mc)
    {
        std::string ev="resume_mc";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::hex << mc;
            log(timestamp, id, 0, sstream.str());
        }
    }

    void Logger::logResumeWithMemBank(uint64_t timestamp, uint64_t id, uint64_t mem_bank)
    {
        std::string ev="resume_memory_bank";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::dec << mem_bank;
            log(timestamp, id, 0, sstream.str());
        }
    }

    void Logger::logResumeWithCacheBank(uint64_t timestamp, uint64_t id, uint64_t cache_bank)
    {
        std::string ev="resume_cache_bank";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::dec << cache_bank;
            log(timestamp, id, 0, sstream.str());
        }
    }

    void Logger::logResumeWithTile(uint64_t timestamp, uint64_t id, uint64_t tile_id)
    {
        std::string ev="resume_tile";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::dec << tile_id;
            log(timestamp, id, 0, sstream.str());
        }
    }

    void Logger::logResumeWithAddress(uint64_t timestamp, uint64_t id, uint64_t address)
    {
        std::string ev="resume_address";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::hex << address;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logResume(uint64_t timestamp, uint64_t id)
    {
        std::string ev="resume";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0,0";
            log(timestamp, id, 0, sstream.str());
        }
    }

    void Logger::logL2Read(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size)
    {
        std::string ev="l2_read";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream <<  ev  << "," << size << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logL2Write(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size)
    {
        std::string ev="l2_write";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream <<  ev  << "," << size << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }
    
    void Logger::logLLCRead(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size)
    {
        std::string ev="llc_read";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev  << "," << size << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logLLCWrite(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size)
    {
        std::string ev="llc_write";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev  << "," << size << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logStall(uint64_t timestamp, uint64_t id, StallReason reason)
    {
        std::string ev="stall";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev  << "," << 0 << "," << utils::reason_to_string(reason);
            log(timestamp, id, 0, sstream.str());
        }
    }

    void Logger::logL2Miss(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="l2_miss";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }
    
    void Logger::logL2Hit(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="l2_hit";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logL2WB(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint32_t size)
    {
        std::string ev="l2_wb";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << size << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logLocalBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        std::string ev="local_request";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(bank) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logSurrogateBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t bank, uint64_t address)
    {
        std::string ev="surrogate_request";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(bank) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logRemoteBankRequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t tile, uint64_t address)
    {
        std::string ev="remote_request";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryCPURequest(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address)
    {
        std::string ev="memory_request";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(mc) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryCPUOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="memory_operation";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << 0 << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logNoCMessageSource(uint64_t timestamp, uint64_t core_id, uint64_t src_id, uint64_t pc)
    {
        std::string ev="noc_message_src";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << src_id;
            log(timestamp, core_id, pc, sstream.str());
        }
    }
   
    void Logger::logNoCMessageDestination(uint64_t timestamp, uint64_t core_id, uint64_t dst_id, uint64_t pc)
    {
        std::string ev="noc_message_dst";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << ",0," << dst_id;
            log(timestamp, core_id, pc, sstream.str());
        }
    }
    
    void Logger::logMemoryCPUAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::string ev="memory_ack";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryControllerRead(uint64_t timestamp, uint64_t id, uint64_t pc, uint32_t size, uint64_t address)
    {
        std::string ev="memory_read";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(size) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }
    
    void Logger::logMemoryControllerWrite(uint64_t timestamp, uint64_t id, uint64_t pc, uint32_t size, uint64_t address)
    {
        std::string ev="memory_write";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(size) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryControllerOperation(uint64_t timestamp, uint64_t id, uint64_t pc, uint8_t mc, uint64_t address)
    {
        std::string ev="memory_operation";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(mc) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }
    
    void Logger::logMemoryBankCommand(uint64_t timestamp, uint64_t mc, uint64_t pc, uint8_t bank, uint64_t address)
    {
        std::string ev="bank_operation";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(bank) << "," << std::hex << address;
            log(timestamp, mc, pc, sstream.str());
        }
    }

    void Logger::logMissOnEvicted(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address, uint64_t time_since_eviction)
    {
        std::string ev="miss_on_evicted";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(time_since_eviction) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMemoryControllerAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::string ev="memory_ack";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logTileRecAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="ack_received";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev <<",0," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logTileSendAck(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t tile, uint64_t address)
    {
        std::string ev="ack_forwarded";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << unsigned(tile) << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logTileRecAckForwarded(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="ack_forward_received";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << 0 << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }

    void Logger::logMissServiced(uint64_t timestamp, uint64_t id, uint64_t pc, uint64_t address)
    {
        std::string ev="miss_serviced";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
            std::stringstream sstream;
            sstream << ev << "," << 0 << "," << std::hex << address;
            log(timestamp, id, pc, sstream.str());
        }
    }
    
    void Logger::logKI(uint64_t timestamp, uint64_t id)
    {
        std::string ev="KI";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp))
        {
           log(timestamp, id, 0, "KI,0,0");
        }
    }
    
    //-- Memory Tile Logs
    void Logger::logMemTileOccupancyOutNoC(uint64_t timestamp, uint16_t id, uint32_t value) {
        std::string ev = "mem_tile_occupancy_out_noc";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << value;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileOccupancyMC(uint64_t timestamp, uint16_t id, uint32_t value) {
        std::string ev = "mem_tile_occupancy_mc";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << value;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileVVL(uint64_t timestamp, uint16_t id, uint64_t core_id, uint32_t value) {
        std::string ev = "mem_tile_vvl";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << core_id << "," << value;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileVecOpRecv(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t address) {
        std::string ev="mem_tile_vecop_recv";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << core_id << "," << std::hex << address;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileVecOpSent(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t address) {
        std::string ev="mem_tile_vecop_sent";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << core_id << "," << std::hex << address;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileScaOpRecv(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t address) {
        std::string ev="mem_tile_scaop_recv";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << core_id << "," << std::hex << address;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileScaOpSent(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t address) {
        std::string ev="mem_tile_scaop_sent";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << core_id << "," << std::hex << address;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileSPOpRecv(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t parent_address) {
        std::string ev="mem_tile_spop_recv";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << core_id;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileSPOpSent(uint64_t timestamp, uint16_t id, uint64_t core_id, uint64_t parent_address) {
        std::string ev="mem_tile_spop_sent";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << core_id;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileMTOpRecv(uint64_t timestamp, uint16_t id, uint16_t src_id, uint64_t address) {
        std::string ev="mem_tile_mtop_recv";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << src_id << "," << std::hex << address;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileMTOpSent(uint64_t timestamp, uint16_t id, uint16_t dest_id, uint64_t address) {
        std::string ev="mem_tile_mtop_sent";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << dest_id << "," << std::hex << address;
            log(timestamp, id, 0, sstream.str());
        }
    }
    
    void Logger::logMemTileMCRecv(uint64_t timestamp, uint16_t id, uint64_t address) {
        logMemTileMCRecv(timestamp, id, address, 0);
    }
    void Logger::logMemTileMCRecv(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address) {
        std::string ev="mem_tile_mc_recv";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << std::hex << address;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileMCSent(uint64_t timestamp, uint16_t id, uint64_t address) {
        logMemTileMCSent(timestamp, id, address, 0);
    }
    void Logger::logMemTileMCSent(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address) {
        std::string ev="mem_tile_mc_sent";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << std::hex << address;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileLLCRecv(uint64_t timestamp, uint16_t id, uint64_t address) {
        logMemTileLLCRecv(timestamp, id, address, 0);
    }
    void Logger::logMemTileLLCRecv(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address) {
        std::string ev="mem_tile_llc_recv";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << std::hex << address;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileLLCSent(uint64_t timestamp, uint16_t id, uint64_t address) {
        logMemTileLLCSent(timestamp, id, address, 0);
    }
    void Logger::logMemTileLLCSent(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address) {
        std::string ev="mem_tile_llc_sent";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << std::hex << address;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileLLC2MC(uint64_t timestamp, uint16_t id, uint64_t address) {
        logMemTileLLC2MC(timestamp, id, address, 0);
    }
    void Logger::logMemTileLLC2MC(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address) {
        std::string ev="mem_tile_llc2mc";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << std::hex << address;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileMC2LLC(uint64_t timestamp, uint16_t id, uint64_t address) {
        logMemTileMC2LLC(timestamp, id, address, 0);
    }
    void Logger::logMemTileMC2LLC(uint64_t timestamp, uint16_t id, uint64_t address, uint64_t parent_address) {
        std::string ev="mem_tile_mc2llc";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << std::hex << address;
            log(timestamp, id, parent_address, sstream.str());
        }
    }
    
    void Logger::logMemTileNoCRecv(uint64_t timestamp, uint16_t id, uint16_t srcAddress, uint64_t address) {
        std::string ev="mem_tile_noc_recv";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << srcAddress;
            log(timestamp, id, address, sstream.str());
        }
    }
    
    void Logger::logMemTileNoCSent(uint64_t timestamp, uint16_t id, uint16_t destAddress, uint64_t address) {
        std::string ev="mem_tile_noc_sent";
        if(checkIfEventOfInterest(ev) && checkBounds(timestamp)) {
            std::stringstream sstream;
            sstream << ev << "," << id << "," << destAddress;
            log(timestamp, id, address, sstream.str());
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
    
    void Logger::setTimeBounds(uint64_t lower, uint64_t upper)
    {
        lower_bound_=lower;
        upper_bound_=upper;
    }
            
    bool Logger::checkBounds(uint64_t t)
    {
        return t>=lower_bound_ && t<upper_bound_;
    }
            
    void Logger::addEventOfInterest(uint64_t start, uint64_t end)
    {
        start=start;
        end=end;
    }
}
