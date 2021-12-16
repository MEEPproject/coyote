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
    delay_open(p->delay_open),
    delay_close(p->delay_close),
    delay_read(p->delay_read),
    delay_write(p->delay_write)
    {

    }

    void MemoryBank::issue(std::shared_ptr<BankCommand> c)
    {
        uint64_t delay=0;
        switch(c->getType())
        {
            case BankCommand::CommandType::ACTIVATE:
                count_open_++;
                state=BankState::OPENING;
                delay=delay_open;
                break;

            case BankCommand::CommandType::PRECHARGE:
                count_close_++;
                state=BankState::CLOSING;
                delay=delay_close;
                break;

            case BankCommand::CommandType::READ:
                count_read_++;
                state=BankState::READING;
                delay=delay_read * c->getRequest()->get_mem_op_latency();   // The HBM returns <= 32B. For larger requests, the delay is adapted.
                break;

            case BankCommand::CommandType::WRITE:
                count_write_++;
                state=BankState::WRITING;
                delay=delay_write * c->getRequest()->get_mem_op_latency();  // The HBM returns <= 32B. For larger requests, the delay is adapted.
                break;
        }
        command_completed_event.preparePayload(c)->schedule(delay+1);

        if(trace_)
        {
            logger_.logMemoryBankCommand(getClock()->currentCycle(), c->getRequest()->getMemoryController(), c->getRequest()->getPC(), c->getDestinationBank(), c->getRequest()->getAddress());
        }
    }

    uint64_t MemoryBank::getOpenRow()
    {
        return current_row;
    }
    
    bool MemoryBank::isReady()
    {
        return state==BankState::OPEN || state==BankState::CLOSED;
    }
    
    bool MemoryBank::isOpen()
    {
        return state==BankState::OPEN;
    }
    
    void MemoryBank::notifyCompletion(const std::shared_ptr<BankCommand>& c)
    {
        switch(c->getType())
        {
            case BankCommand::CommandType::ACTIVATE:
                state=BankState::OPEN;
                current_row=c->getValue();
                break;

            case BankCommand::CommandType::PRECHARGE:
                state=BankState::CLOSED;
                break;

            case BankCommand::CommandType::READ:
                state=BankState::OPEN;
                break;

            case BankCommand::CommandType::WRITE:
                state=BankState::OPEN;
                break;
        }
        mc->notifyCompletion_(c);
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
}
