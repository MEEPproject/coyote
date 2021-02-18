#include "FixedLatencyDispatcher.hpp"

#include "SpikeModelTypes.hpp"

#include "SpikeModel.hpp"

namespace spike_model
{
    FixedLatencyDispatcher::FixedLatencyDispatcher(sparta::TreeNode * node, const DispatchParameterSet * p) : AbstractDispatcher::AbstractDispatcher(node, p) 
    {

    }


    void FixedLatencyDispatcher::dispatchInstructions_()
    {
        uint32_t num_dispatch = num_to_dispatch_;

        if(SPARTA_EXPECT_FALSE(info_logger_)) {
            info_logger_ << "Num to dispatch: " << num_dispatch;
        }

        // Stop the current counter
        stall_counters_[current_stall_].stopCounting();

        if(num_dispatch == 0) {
            stall_counters_[current_stall_].startCounting();
            return;
        }

        current_stall_ = NOT_STALLED;

        InstGroup insts_dispatched;
        bool keep_dispatching = !finished;
        num_dispatched_since_last_ev=0;
        RegisterAvailability raw_satisfied;
        while(keep_dispatching)
        {
            //printf("Inside loop with %d\n", i);
        
            bool dispatched=false;
    
            std::shared_ptr<BaseInstruction> ex_inst_ptr;

            if(pending_inst!=NULL)
            {
                ex_inst_ptr=pending_inst;
                pending_inst=NULL;
            }
            else
            {
                ex_inst_ptr=fetch_->fetchInstruction_();
               // std::cout << "Got instruction " << *ex_inst_ptr << "\n";
            }


            uint8_t rs1=ex_inst_ptr->getRs1();
            uint8_t rs2=ex_inst_ptr->getRs2();
            uint8_t rs3=ex_inst_ptr->getRs3();
            uint8_t rd=ex_inst_ptr->getRd();            

            
            bool raw=checkRawAndUpdate(rs1, rs2, rs3, raw_satisfied);        
    
            if(!raw)
            {
                dispatched=ex_inst_ptr->sendToDispatcher(*this);
                //printf("Dispatched %d\n", dispatched);
            }
            else
            {
                current_stall_ = RAW;
                pending_inst=ex_inst_ptr;
                //printf("RAW!!!!!!!!!!!!!W\n");
            }
 
            if(dispatched) 
            {
                num_dispatched_since_last_ev++;
                if(rd!=0) //Register 0 does not get written
                {
                    markRegisterAsUsed(rd);
                }
            } 
            else 
            {
                if(SPARTA_EXPECT_FALSE(info_logger_))
                {
                    info_logger_ << "Could not dispatch: "
                                   << ex_inst_ptr
                                 << ") RAW (";
                }
            }
            keep_dispatching=dispatched && !finished;
        }
        
        if(!finished)
        {
            //printf("Im in RAW\n");
            if(raw_satisfied.cycle!=RegisterAvailability::EVENT_DEPENDENT)
            {
                //printf("Scheduling next for reg %u\n", raw_satisfied.reg);
                ev_dispatch_insts_.schedule(raw_satisfied.cycle-(num_dispatched_since_last_ev+getClock()->currentCycle()));
            }
            else
            {
                reg_expected_for_raw=raw_satisfied.reg;
                current_stall_=WAITING_ON_MEMORY;
                //printf("Waiting for memory on reg %u\n", reg_expected_for_raw);
            }
        }

        stall_counters_[current_stall_].startCounting();
    }



    bool FixedLatencyDispatcher::dispatch(FixedLatencyInstruction& i) 
    {
        //std::cout << "Going to issue inst " << i << "\n";
        bool dispatched=false;
        dispatched = true;
        latency_last_inst_=execute_->issueInst_(i);
        ++unit_distribution_[i.getId()];

        if(SPARTA_EXPECT_FALSE(info_logger_))
        {
            info_logger_ << "Sending instruction: "
                         << i << " to Fixed Latency ";
        }

        return dispatched;
    }

    bool FixedLatencyDispatcher::dispatch(MemoryInstruction& i) 
    {
        ++unit_distribution_[i.getId()];
        FixedLatencyInstruction j (i.getBits());
        j.setRd(i.getRd());
        j.setRs1(i.getRs1());
        j.setRs2(i.getRs2());
        //std::cout << "Instruction " << i << "is a memory instruction that generates " << i.getAccesses().size() << "accesses and has opcode\n";
        //printf("---------> Opcode %u\n", i.getOpcode());
        return dispatch(j);
    }

    bool FixedLatencyDispatcher::dispatch(StateInstruction& i) 
    {
        if(i.getType()==StateInstruction::Type::FINISH)
        {
            finished=true;
            
            SpikeModel * spike= dynamic_cast<SpikeModel *> (getContainer()->getSimulation()); //Not a huge fan of downcasting, but alternatives seem convolutes
            uint32_t num_running=spike->notifyCoreCompletion();
            if(num_running==0)
            {
                //getScheduler()->stopRunning();
            }

        }
        return true;
    }

    bool FixedLatencyDispatcher::dispatch(SynchronizationInstruction& i) 
    {
        FixedLatencyInstruction j (0);
        return dispatch(j);
    }
}
