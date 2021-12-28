#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryBank.hpp"
#include "CacheRequest.hpp"

namespace spike_model
{
    const char MemoryBank::name[] = "memory_controller";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    MemoryBank::MemoryBank(sparta::TreeNode *node, const MemoryBankParameterSet *p):
    sparta::Unit(node),
    num_rows(p->num_rows),
    num_columns(p->num_columns),
    column_element_size(p->column_element_size),
    burst_length(p->burst_length)
    {
    }

    void MemoryBank::issue(const std::shared_ptr<BankCommand>& c)
    {
        //uint64_t delay=0;
        switch(c->getType())
        {
            case BankCommand::CommandType::ACTIVATE:
                count_activate_++;
                current_row=c->getValue();
                state=BankState::OPEN;
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RRDS]);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RC]);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RAS]+(*latencies)[(int)MemoryController::LatencyName::RP]);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RCDRD]);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RCDWR]);
                //delay=delay_open;
                break;

            case BankCommand::CommandType::PRECHARGE:
                count_precharge_++;
                state=BankState::CLOSED;
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RP]);
                //delay=delay_close;
                break;

            case BankCommand::CommandType::READ:
                count_read_++;
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::CCDS]);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RTW]);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::RTP]);
                //delay=delay_read * c->getRequest()->get_mem_op_latency();   // The HBM returns <= 32B. For larger requests, the delay is adapted.

                data_available_event.preparePayload(c)->schedule((*latencies)[(int)MemoryController::LatencyName::RL]); //This should take into account bursts
                break;

            case BankCommand::CommandType::WRITE:
                count_write_++;
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::CCDS]);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::WR]+(*latencies)[(int)MemoryController::LatencyName::WL]+burst_length);
                timing_event.schedule((*latencies)[(int)MemoryController::LatencyName::WTRL]+(*latencies)[(int)MemoryController::LatencyName::WL]+burst_length);
                //delay=(*latencies)[(int)MemoryController::LatencyName::WL]//delay_write * c->getRequest()->get_mem_op_latency();  // The HBM returns <= 32B. For larger requests, the delay is adapted.
                
                data_available_event.preparePayload(c)->schedule((*latencies)[(int)MemoryController::LatencyName::WL]); //This should take into account bursts
                break;

            case BankCommand::CommandType::NUM_COMMANDS:
                std::cout << "Commands of type NUM_COMMANDS should not be issued\n";
                break;
        }

        if(trace_)
        {
            logger_.logMemoryBankCommand(getClock()->currentCycle(), c->getRequest()->getMemoryController(), c->getRequest()->getPC(), c->getDestinationBank(), c->getRequest()->getAddress());
        }
    }

    uint64_t MemoryBank::getOpenRow()
    {
        return current_row;
    }
    
    bool MemoryBank::isOpen()
    {
        return state==BankState::OPEN;
    }
    
    void MemoryBank::notifyTimingEvent()
    {
        mc->notifyTimingEvent();
    }
    
    void MemoryBank::notifyDataAvailable(const std::shared_ptr<BankCommand>& c)
    {
        mc->notifyDataAvailable_(c);
    }
            
    void MemoryBank::setMemoryController(MemoryController * controller)
    {
        mc=controller;
    }
            
    uint64_t MemoryBank::getNumRows()
    {
        return num_rows;
    }
    
    uint64_t MemoryBank::getNumColumns()
    {
        return num_columns;
    }
    
    void MemoryBank::setMemSpec(std::shared_ptr<std::vector<uint64_t>> spec) 
    {
        latencies=spec;
    }
            
    uint64_t MemoryBank::getBurstSize()
    {
        return column_element_size*burst_length;
    }
    
    uint8_t MemoryBank::getBurstLength()
    {
        return burst_length;
    }
}
