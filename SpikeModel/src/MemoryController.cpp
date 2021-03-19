#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryController.hpp"
#include "FifoRrMemoryAccessScheduler.hpp"
#include "FifoCommandScheduler.hpp"

namespace spike_model
{
    const char MemoryController::name[] = "memory_controller";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    MemoryController::MemoryController(sparta::TreeNode *node, const MemoryControllerParameterSet *p):
    sparta::Unit(node),
    latency_(p->latency),
    line_size_(p->line_size),
    num_banks_(p->num_banks),
    write_allocate_(p->write_allocate),
    banks()
    {
        in_port_noc_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryController, receiveMessage_, std::shared_ptr<NoCMessage>));
        sched=std::make_unique<FifoRrMemoryAccessScheduler>(num_banks_);
        ready_commands=std::make_unique<FifoCommandScheduler>();
    }

    void MemoryController::receiveMessage_(const std::shared_ptr<NoCMessage> & mes)
    {
        count_requests_++;
        mes->getRequest()->handle(this);
    }

    void MemoryController::handle(std::shared_ptr<spike_model::CacheRequest> mes)
    {
        uint64_t bank=mes->getMemoryBank();
        sched->putRequest(mes, bank);
        if(idle_ & sched->hasIdleBanks())
        {
            controller_cycle_event_.schedule();
            idle_=false;
        }
    }

    void MemoryController::handle(std::shared_ptr<spike_model::MCPURequest> r)
    {
        std::cout << "Requesting vec len from MCPU from core " << r->getCoreId()  << " and vector len " << r->getRequestedVecLen() << std::endl;
        mcpu_req.push_back(r);
        issue_mcpu_event_.schedule(1);
    }

    void MemoryController::issueAck_(std::shared_ptr<CacheRequest> req)
    {
        //std::cout << "Issuing ack from memory controller to request from core " << mes->getRequest()->getCoreId() << " for address " << mes->getRequest()->getAddress() << "\n";
        req->setMemoryAck(true);
        out_port_noc_.send(std::make_shared<NoCMessage>(req, NoCMessageType::MEMORY_ACK, line_size_, req->getHomeTile()), 0);
    }
    
    std::shared_ptr<BankCommand> MemoryController::getAccessCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        std::shared_ptr<BankCommand> res_command;

        uint64_t column_to_schedule=req->getCol();
        if(req->getType()==CacheRequest::AccessType::STORE || req->getType()==CacheRequest::AccessType::WRITEBACK)
        {
            res_command=std::make_shared<BankCommand>(BankCommand::CommandType::WRITE, bank, column_to_schedule);
        }
        else
        {
            res_command=std::make_shared<BankCommand>(BankCommand::CommandType::READ, bank, column_to_schedule);
        }
        return res_command;
    }
    
    std::shared_ptr<BankCommand> MemoryController::getAllocateCommand_(std::shared_ptr<CacheRequest> req, uint64_t bank)
    {
        sparta_assert(req->getType()==CacheRequest::AccessType::STORE && write_allocate_, "Allocates can only by submitted for stores and when allocation is enabled\n");
        
        std::shared_ptr<BankCommand> res_command;

        uint64_t column_to_schedule=req->getCol();
        res_command=std::make_shared<BankCommand>(BankCommand::CommandType::READ, bank, column_to_schedule);
        return res_command;
    }

    void MemoryController::issueMCPU_()
    {
        std::shared_ptr<MCPURequest> m = mcpu_req.front();
        m->setReturnedVecLen(m->getRequestedVecLen());
        std::cout << "Returning vec len from MCPU from core " << m->getCoreId() << " and vector len " << m->getReturnedVecLen() << std::endl;
        m->setServiced();
        mcpu_req.pop_front();
        out_port_noc_.send(std::make_shared<NoCMessage>(m, NoCMessageType::MCPU_REQUEST, line_size_, m->getSourceTile()), 0);
    }

    void MemoryController::controllerCycle_()
    {
        while(sched->hasIdleBanks())
        {
            uint64_t bank_to_schedule=sched->getNextBank();
            std::shared_ptr<CacheRequest> request_to_schedule=sched->getRequest(bank_to_schedule);
            uint64_t row_to_schedule=request_to_schedule->getRow();

            std::shared_ptr<BankCommand> com;
            if(banks[bank_to_schedule]->isOpen() && banks[bank_to_schedule]->getOpenRow()==row_to_schedule)
            {
                com=getAccessCommand_(request_to_schedule, bank_to_schedule);
            }
            else
            {
               if(banks[bank_to_schedule]->isOpen())
               {
                    com=std::make_shared<BankCommand>(BankCommand::CommandType::CLOSE, bank_to_schedule, 0);
               }
               else
               {
                    com=std::make_shared<BankCommand>(BankCommand::CommandType::OPEN, bank_to_schedule, row_to_schedule);
               }
            }
            ready_commands->addCommand(com);
        }

        if(ready_commands->hasCommands())
        {
            std::shared_ptr<BankCommand> next=ready_commands->getNextCommand();
            banks[next->getDestinationBank()]->issue(next);
            if(ready_commands->hasCommands())
            {
                controller_cycle_event_.schedule(1);
            }
            else
            {
                idle_=true;
            }
        }
        else
        {
        }
    }
            
    void MemoryController::addBank_(MemoryBank * bank)
    {
        banks.push_back(bank);
    } 
    
    void MemoryController::notifyCompletion_(std::shared_ptr<BankCommand> c)
    {
        std::shared_ptr<BankCommand> com=nullptr;
        uint64_t command_bank=c->getDestinationBank();
        switch(c->getType())
        {
            case BankCommand::CommandType::OPEN:
            {
                std::shared_ptr<CacheRequest> pending_request_for_bank=sched->getRequest(command_bank);
                com=getAccessCommand_(pending_request_for_bank, command_bank);
                break;
            }

            case BankCommand::CommandType::CLOSE:
            {
                std::shared_ptr<CacheRequest> pending_request_for_bank=sched->getRequest(command_bank);
                uint64_t row_to_schedule=pending_request_for_bank->getRow();
                com=std::make_shared<BankCommand>(BankCommand::CommandType::OPEN, command_bank, row_to_schedule);
                break;
            }

            case BankCommand::CommandType::READ:
            {
                std::shared_ptr<CacheRequest> pending_request_for_bank=sched->getRequest(command_bank);
                sched->notifyRequestCompletion(command_bank);
                if(pending_request_for_bank->getType()==CacheRequest::AccessType::LOAD || pending_request_for_bank->getType()==CacheRequest::AccessType::FETCH || (pending_request_for_bank->getType()==CacheRequest::AccessType::STORE && write_allocate_))
                {
                    issueAck_(pending_request_for_bank);
                }
                break;
            }
           
            case BankCommand::CommandType::WRITE:
            {
                std::shared_ptr<CacheRequest> pending_request_for_bank=sched->getRequest(command_bank);

                if(!write_allocate_ || sched->getRequest(command_bank)->getType()==CacheRequest::AccessType::WRITEBACK)
                {
                    sched->notifyRequestCompletion(command_bank);
                }
                else
                {
                    com=getAllocateCommand_(pending_request_for_bank, command_bank);
                }
                break;
            }
        }
        
        if(com!=nullptr)
        {
            ready_commands->addCommand(com);
        }
       
        if(idle_ && (sched->hasIdleBanks() || ready_commands->hasCommands()))
        {
            controller_cycle_event_.schedule();
            idle_=false;
        }
    }
    
    void MemoryController::setRequestManager(std::shared_ptr<EventManager> r)
    {
        request_manager_=r;
    }
}
