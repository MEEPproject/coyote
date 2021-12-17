#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryController.hpp"
#include "FifoRrMemoryAccessScheduler.hpp"
#include "FifoRrMemoryAccessSchedulerAccessTypePriority.hpp"
#include "FifoCommandScheduler.hpp"
#include "utils.hpp"

namespace spike_model
{
    const char MemoryController::name[] = "memory_controller";

    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    MemoryController::MemoryController(sparta::TreeNode *node, const MemoryControllerParameterSet *p):
    sparta::Unit(node),
    num_banks_(p->num_banks),
    num_banks_per_group_(p->num_banks_per_group),
    write_allocate_(p->write_allocate),
    reordering_policy_(p->reordering_policy),
    unused_lsbs_(p->unused_lsbs)
    {
        in_port_mcpu_.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryController, receiveMessage_, std::shared_ptr<CacheRequest>));
        ready_commands=std::make_unique<FifoCommandScheduler>();

        banks=std::make_shared<std::vector<MemoryBank *>>();
        
        if(reordering_policy_=="none")
        {
            sched=std::make_unique<FifoRrMemoryAccessScheduler>(banks, num_banks_, write_allocate_);
        }
        else if(reordering_policy_=="access_type")
        {
            sched=std::make_unique<FifoRrMemoryAccessSchedulerAccessTypePriority>(banks, num_banks_, write_allocate_);
        }
        else
        {
            std::cout << "Unsupported reordering policy. Falling back to the default policy.\n";
            sched=std::make_unique<FifoRrMemoryAccessSchedulerAccessTypePriority>(banks, num_banks_, write_allocate_);
        }

        address_mapping_policy_=spike_model::AddressMappingPolicy::OPEN_PAGE;
        if(p->address_policy=="open_page")
        {
            address_mapping_policy_=spike_model::AddressMappingPolicy::OPEN_PAGE;
        }
        else if(p->address_policy=="close_page")
        {
            address_mapping_policy_=spike_model::AddressMappingPolicy::CLOSE_PAGE;
        }
        else if(p->address_policy=="row_bank_column_bank_group_interleave")
        {
            address_mapping_policy_=spike_model::AddressMappingPolicy::ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE;
        }
        else if(p->address_policy=="row_column_bank")
        {
            address_mapping_policy_=spike_model::AddressMappingPolicy::ROW_COLUMN_BANK;
        }
        else if(p->address_policy=="bank_row_column")
        {
            address_mapping_policy_=spike_model::AddressMappingPolicy::BANK_ROW_COLUMN;
        }
        else
        {
            printf("Unsupported address data mapping policy\n");
        }

    }


    void MemoryController::receiveMessage_(const std::shared_ptr<spike_model::CacheRequest> &mes)
    {
        uint64_t address=mes->getAddress();
        
        uint64_t rank=0;
        if(rank_mask!=0)
        {
            rank=calculateRank(address);
        }

        uint64_t bank=0;
        if(bank_mask!=0)
        {
            bank=calculateBank(address);
        }

        uint64_t row=0;
        if(row_mask!=0)
        {
            row=calculateRow(address);
        }

        uint64_t col=0;
        if(col_mask!=0)
        {
            col=calculateCol(address);
        }
        
        mes->setBankInfo(rank, bank, row, col);

        mes->setTimestampReachMC(getClock()->currentCycle());

        sched->putRequest(mes, bank);
        if(trace_)
        {
            logger_.logMemoryControllerRequest(getClock()->currentCycle(), mes->getCoreId(), mes->getPC(), mes->getMemoryController(), mes->getAddress());
        }

        if(idle_ & sched->hasBanksToSchedule())
        {
            controller_cycle_event_.schedule();
            idle_=false;
        }
    }


    void MemoryController::issueAck_(std::shared_ptr<CacheRequest> req)
    {
        //std::cout << "Issuing ack from memory controller to request from core " << mes->getRequest()->getCoreId() << " for address " << mes->getRequest()->getAddress() << "\n";
        //out_port_noc_.send(std::make_shared<NoCMessage>(req, NoCMessageType::MEMORY_ACK, line_size_, req->getHomeTile()), 0);
        if(req->getType()==CacheRequest::AccessType::LOAD || req->getType()==CacheRequest::AccessType::FETCH)
        {
            count_read_requests_++;
            total_time_spent_by_read_requests_=total_time_spent_by_read_requests_+(getClock()->currentCycle()-req->getTimestampReachMC());
        }
        else
        {
            count_write_requests_++;
            total_time_spent_by_write_requests_=total_time_spent_by_write_requests_+(getClock()->currentCycle()-req->getTimestampReachMC());
        }
        
        out_port_mcpu_.send(req, 0);
    }
    
    void MemoryController::controllerCycle_()
    {
        uint64_t current_t=getClock()->currentCycle();
        average_queue_occupancy_=((float)(average_queue_occupancy_*last_queue_sampling_timestamp)/current_t)+((float)sched->getQueueOccupancy()*(current_t-last_queue_sampling_timestamp))/current_t;
        max_queue_occupancy_= (max_queue_occupancy_>=sched->getQueueOccupancy()) ? max_queue_occupancy_ : sched->getQueueOccupancy();
        last_queue_sampling_timestamp=current_t;
        while(sched->hasBanksToSchedule())
        {
            uint64_t bank_to_schedule=sched->getNextBank();
            std::shared_ptr<BankCommand> command_to_schedule=sched->getCommand(bank_to_schedule);
            if(trace_)
            {
                logger_.logMemoryControllerOperation(current_t, command_to_schedule->getRequest()->getCoreId(), command_to_schedule->getRequest()->getPC(), command_to_schedule->getRequest()->getMemoryController(), command_to_schedule->getRequest()->getAddress());

            }
        
            if(command_to_schedule->getRequest()->getType()==CacheRequest::AccessType::LOAD || command_to_schedule->getRequest()->getType()==CacheRequest::AccessType::FETCH)
            {
                total_time_spent_in_queue_read_=total_time_spent_in_queue_read_+(current_t-command_to_schedule->getRequest()->getTimestampReachMC());
            }
            else
            {
                total_time_spent_in_queue_write_=total_time_spent_in_queue_write_+(current_t-command_to_schedule->getRequest()->getTimestampReachMC());
            }
            ready_commands->addCommand(command_to_schedule);
        }

        //Add back to the sched banks that did not get any commands

        if(ready_commands->hasCommands())
        {
            std::shared_ptr<BankCommand> next=ready_commands->getNextCommand();
            (*banks)[next->getDestinationBank()]->issue(next);
            if(ready_commands->hasCommands())
            {
                controller_cycle_event_.schedule(1);
            }
            else
            {
                idle_=true;
            }
        }
    }


    void MemoryController::addBank_(MemoryBank * bank)
    {
        banks->push_back(bank);
    }


    void MemoryController::notifyTimingEvent()
    {
        if(idle_ && sched->hasBanksToSchedule())
        {
            controller_cycle_event_.schedule();
            idle_=false;
        } 
    }

    void MemoryController::notifyCompletion_(std::shared_ptr<BankCommand> c)
    {
        std::shared_ptr<CacheRequest> serviced_request=sched->notifyCommandCompletion(c);

        if(serviced_request!=nullptr)
        {
            issueAck_(serviced_request);
        }

        if(idle_ && (sched->hasBanksToSchedule() || ready_commands->hasCommands()))
        {
            controller_cycle_event_.schedule();
            idle_=false;
        }
    }

    spike_model::AddressMappingPolicy MemoryController::getAddressMapping()
    {
        return address_mapping_policy_;
    }
    
    uint64_t MemoryController::calculateRank(uint64_t address)
    {
        uint64_t res=0;
        switch(address_mapping_policy_)
        {
            case AddressMappingPolicy::OPEN_PAGE:
                res=(address >> rank_shift) & rank_mask;
                break;    
            case AddressMappingPolicy::CLOSE_PAGE:
                res=(address >> rank_shift) & rank_mask;
                break;
            case spike_model::AddressMappingPolicy::ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE:
                res=(address >> rank_shift) & rank_mask;
                break;
            case spike_model::AddressMappingPolicy::ROW_COLUMN_BANK:
                res=(address >> rank_shift) & rank_mask;
                break;
            case spike_model::AddressMappingPolicy::BANK_ROW_COLUMN:
                res=(address >> rank_shift) & rank_mask;
                break;
        }
        return res;
    }

    uint64_t MemoryController::calculateRow(uint64_t address)
    {
        uint64_t res=0;
        switch(address_mapping_policy_)
        {
            case AddressMappingPolicy::OPEN_PAGE:
                res=(address >> row_shift) & row_mask;
                break;    
            case AddressMappingPolicy::CLOSE_PAGE:
                res=(address >> row_shift) & row_mask;
                break;
            case spike_model::AddressMappingPolicy::ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE:
                res=(address >> row_shift) & row_mask;
                break;
            case spike_model::AddressMappingPolicy::ROW_COLUMN_BANK:
                res=(address >> row_shift) & row_mask;
                break;
            case spike_model::AddressMappingPolicy::BANK_ROW_COLUMN:
                res=(address >> row_shift) & row_mask;
                break;
        }
        return res;
    }
    
    uint64_t MemoryController::calculateBank(uint64_t address)
    {
        uint64_t res=0;
        switch(address_mapping_policy_)
        {
            case AddressMappingPolicy::OPEN_PAGE:
                res=(address >> bank_shift) & bank_mask;
                break;    
            case AddressMappingPolicy::CLOSE_PAGE:
                res=(address >> bank_shift) & bank_mask;
                break;
            case spike_model::AddressMappingPolicy::ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE:
            {
                uint64_t num_groups=num_banks_/num_banks_per_group_;
                if(num_groups!=1)
                {
                    uint64_t group_low=(address >> unused_lsbs_) & 1;
                    uint64_t bank=(address >> bank_shift) & (num_banks_per_group_-1);
                    uint64_t group_high=(address >> (bank_shift+(uint64_t)ceil(log2(num_banks_per_group_)))) & ((uint64_t)(num_groups/2)-1);
                    uint64_t group_shift=ceil(log2(num_banks_per_group_));
                    res= (group_high << (group_shift+1)) | (group_low << group_shift) | bank;
                }
                else
                {
                    res=(address >> bank_shift) & (num_banks_per_group_-1);
                }
                break;
            }
            case spike_model::AddressMappingPolicy::ROW_COLUMN_BANK:
                res=(address >> bank_shift) & bank_mask;
                break;
            case spike_model::AddressMappingPolicy::BANK_ROW_COLUMN:
                res=(address >> bank_shift) & bank_mask;
                break;
        }
        return res;

    }
    
    uint64_t MemoryController::calculateCol(uint64_t address)
    {
        uint64_t res=0;
        switch(address_mapping_policy_)
        {
            case AddressMappingPolicy::OPEN_PAGE:
                res=(address >> col_shift) & col_mask;
                break;    
            case AddressMappingPolicy::CLOSE_PAGE:
                res=(address >> col_shift) & col_mask;
                break;
            case spike_model::AddressMappingPolicy::ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE:
                res=((address >> col_shift) & col_mask)*2;
                break;
            case spike_model::AddressMappingPolicy::ROW_COLUMN_BANK:
                res=((address >> col_shift) & col_mask)*2;
                break;
            case spike_model::AddressMappingPolicy::BANK_ROW_COLUMN:
                res=((address >> col_shift) & col_mask)*2;
                break;
        }
        return res;

    }

    void MemoryController::setup_masks_and_shifts_(uint64_t num_mcs, uint64_t num_rows_per_bank, uint64_t num_cols_per_bank, uint16_t line_size)
    {
        this->line_size=line_size;
        uint64_t mc_shift;
        switch(address_mapping_policy_)
        {
            case AddressMappingPolicy::OPEN_PAGE:
                mc_shift=ceil(log2(line_size));
                col_shift=mc_shift+ceil(log2(num_cols_per_bank));
                bank_shift=col_shift+ceil(log2(num_banks_));
                rank_shift=bank_shift+0;
                row_shift=rank_shift+ceil(log2(num_rows_per_bank));
                
                col_mask=utils::nextPowerOf2(num_cols_per_bank)-1;
                break;    
            case AddressMappingPolicy::CLOSE_PAGE:
                mc_shift=ceil(log2(line_size));
                bank_shift=mc_shift+ceil(log2(num_banks_));
                rank_shift=bank_shift+0;
                col_shift=rank_shift+ceil(log2(num_cols_per_bank));
                row_shift=col_shift+ceil(log2(num_rows_per_bank));
                
                col_mask=utils::nextPowerOf2(num_cols_per_bank)-1;
                break;
            case spike_model::AddressMappingPolicy::ROW_BANK_COLUMN_BANK_GROUP_INTERLEAVE:
            {
                uint64_t num_groups=num_banks_/num_banks_per_group_;
                col_shift=unused_lsbs_;
                if(num_groups!=1)
                {
                    col_shift++;
                }
                bank_shift=col_shift+ceil(log2(num_cols_per_bank))-1; //-1 because the lsbs_ of the col is not addressed

                row_shift=col_shift+ceil(log2(num_cols_per_bank))-1+ceil(log2(num_banks_)); //-1 because the lsbs_ of the col is not addressed
                if(num_groups!=1)
                {
                    row_shift--; // -1 because the bank bits are split
                }
                
                col_mask=utils::nextPowerOf2(num_cols_per_bank/2)-1;
                break;
            }
            case spike_model::AddressMappingPolicy::ROW_COLUMN_BANK:
                bank_shift=unused_lsbs_;
                col_shift=bank_shift+ceil(log2(num_banks_));
                row_shift=col_shift+ceil(log2(num_cols_per_bank))-1;//-1 because the lsbs_ of the col is not addressed
                
                col_mask=utils::nextPowerOf2(num_cols_per_bank/2)-1;
                break;
            case spike_model::AddressMappingPolicy::BANK_ROW_COLUMN:
                col_shift=unused_lsbs_;
                row_shift=col_shift+ceil(log2(num_cols_per_bank))-1;
                bank_shift=row_shift+ceil(log2(num_rows_per_bank));
                
                col_mask=utils::nextPowerOf2(num_cols_per_bank/2)-1;
                break;
        }
        
        rank_mask=0;
        bank_mask=utils::nextPowerOf2(num_banks_)-1;
        row_mask=utils::nextPowerOf2(num_rows_per_bank)-1;
    }    
}
// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab:
