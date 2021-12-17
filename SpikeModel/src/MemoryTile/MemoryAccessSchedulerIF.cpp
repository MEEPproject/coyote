#include "CacheRequest.hpp"
#include "BankCommand.hpp"
#include "MemoryAccessSchedulerIF.hpp"

namespace spike_model
{

    MemoryAccessSchedulerIF::MemoryAccessSchedulerIF(std::shared_ptr<std::vector<MemoryBank *>> b, uint64_t num_banks, bool write_allocate) : banks(b), write_allocate(write_allocate), last_completed_command_per_bank(num_banks)
    {
        for(size_t i=0;i<banks->size();i++)    
        {
            last_completed_command_per_bank[i]=BankCommand::CommandType::PRECHARGE;
        }
    }

    std::shared_ptr<BankCommand> MemoryAccessSchedulerIF::getCommand(uint64_t bank)
    {
        std::shared_ptr<CacheRequest> req=getRequest(bank);

        uint64_t row_to_schedule=req->getRow();

        std::shared_ptr<BankCommand> com;
        if((*banks)[bank]->isOpen()) 
        {
            if((*banks)[bank]->getOpenRow()==row_to_schedule) 
            {
                if(req->getType()==CacheRequest::AccessType::STORE && last_completed_command_per_bank[bank]==BankCommand::CommandType::WRITE && write_allocate)
                {
                    com=getAllocateCommand_(req, bank);
                }
                else
                {
                    com=getAccessCommand_(req, bank);
                }
            } 
            else 
            {
                com=std::make_shared<BankCommand>(BankCommand::CommandType::PRECHARGE, bank, 0, req);
            }
        } 
        else 
        {
            com=std::make_shared<BankCommand>(BankCommand::CommandType::ACTIVATE, bank, row_to_schedule, req);
        }

        return com;
    }
            
    std::shared_ptr<CacheRequest> MemoryAccessSchedulerIF::notifyCommandCompletion(std::shared_ptr<BankCommand> c)
    {
        std::shared_ptr<CacheRequest> serviced_request=nullptr;

        last_completed_command_per_bank[c->getDestinationBank()]=c->getType();

        switch(c->getType())
        {

            case BankCommand::CommandType::READ:
            {
                std::shared_ptr<CacheRequest> pending_request_for_bank=c->getRequest();
                notifyRequestCompletion(pending_request_for_bank);
                if(pending_request_for_bank->getType()==CacheRequest::AccessType::LOAD || pending_request_for_bank->getType()==CacheRequest::AccessType::FETCH || (pending_request_for_bank->getType()==CacheRequest::AccessType::STORE && write_allocate))
                {
                    serviced_request=pending_request_for_bank;
                }
                break;
            }

            case BankCommand::CommandType::WRITE:
            {
                std::shared_ptr<CacheRequest> pending_request_for_bank=c->getRequest();

                if(!write_allocate || c->getRequest()->getType()==CacheRequest::AccessType::WRITEBACK)
                {
                    notifyRequestCompletion(pending_request_for_bank);
                }
                break;
            }

            default:
            {
                break;
            }
        }

        return serviced_request;
    }
    
    std::shared_ptr<BankCommand> MemoryAccessSchedulerIF::getAccessCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        std::shared_ptr<BankCommand> res_command;

        uint64_t column_to_schedule=req->getCol();
        if(req->getType()==CacheRequest::AccessType::STORE || req->getType()==CacheRequest::AccessType::WRITEBACK)
        {
            res_command=std::make_shared<BankCommand>(BankCommand::CommandType::WRITE, bank, column_to_schedule, req);
        }
        else
        {
            res_command=std::make_shared<BankCommand>(BankCommand::CommandType::READ, bank, column_to_schedule, req);
        }
        return res_command;
    }

    std::shared_ptr<BankCommand> MemoryAccessSchedulerIF::getAllocateCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        sparta_assert(req->getType()==CacheRequest::AccessType::STORE && write_allocate, "Allocates can only by submitted for stores and when allocation is enabled\n");
        std::shared_ptr<BankCommand> res_command;

        uint64_t column_to_schedule=req->getCol();
        res_command=std::make_shared<BankCommand>(BankCommand::CommandType::READ, bank, column_to_schedule, req);
        return res_command;
    }
}
