
#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include "AbstractDispatcher.hpp"

#include <string>
#include <array>
#include <cinttypes>

#include "sparta/ports/DataPort.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/events/SingleCycleUniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/log/MessageSource.hpp"
#include "sparta/statistics/Counter.hpp"
#include "sparta/statistics/ContextCounter.hpp"
#include "test/ContextCounter/WeightedContextCounter.hpp"

#include "SpikeModelTypes.hpp"

namespace spike_model
{

    const char AbstractDispatcher::name[] = "dispatch";
    // Constructor
    AbstractDispatcher::AbstractDispatcher(sparta::TreeNode * node,
                       const DispatchParameterSet * p) :
        sparta::Unit(node),
        dispatch_queue_("dispatch_queue", p->dispatch_queue_depth,
                        node->getClock(), getStatisticSet()),
        num_to_dispatch_(p->num_to_dispatch)
    {
        // Start the no instructions counter
        stall_counters_[current_stall_].startCounting();
        dispatch_queue_.enableCollection(node);

        sparta::StartupEvent(node, CREATE_SPARTA_HANDLER(AbstractDispatcher, dispatchInstructions_));
        
        finished=false;
    }    


    //This function returns the latency until a bool indicating if there is a RAW dependency with any of registers 
    //rs1, rs2 and rs3 and sets avail to the availability of the latest of those registers. It also erases any RegisterAvailability
    //object for regs that will become available before that availability.
    bool AbstractDispatcher::checkRawAndUpdate(uint8_t rs1, uint8_t rs2, uint8_t rs3, RegisterAvailability &avail)
    {
        //printf("Checking dependences\n"); 
        std::set<RegisterAvailability>::reverse_iterator rit=register_availability.rbegin();

        bool raw=false;
        uint32_t current_cycle=num_dispatched_since_last_ev+getClock()->currentCycle();
      
        /*if(register_availability.size()>0)
        {
            printf("---------------------------------------------------\n");
            for(RegisterAvailability r: register_availability)
            {
                std::cout << r << ", ";
            }
            std::cout << "\n";
        }*/ 

        while (rit!=register_availability.rend() && !raw) //Go through the (ordered) set starting from the end until a raw is found
        {
           // std::cout << "Checking " << *rit << "\n";
               // std::cout << "---------Register availability has " << register_availability.size() << " entries\n";
            if(rit->cycle!=RegisterAvailability::EVENT_DEPENDENT && rit->cycle<=current_cycle) //If register already available
            {
                //std::cout << "---------Erasing 1. Current cycle is " << unsigned(current_cycle) << "\n";
                rit=std::set<RegisterAvailability>::reverse_iterator{register_availability.erase((++rit).base())}; //Erase and advance
                //std::cout << "---------Register availability has " << register_availability.size() << " entries\n";
            }
            else
            {
                bool found = rit->reg==rs1 || rit->reg==rs2 || rit->reg==rs3; //If there is a match in any of the 3 regs, there is a RAW
                if(found)
                {
                    avail=*rit; //The latest avail found is the one to be kept
                    raw=true;
                   // std::cout << "----------Selected\n";
                }
                else
                {
                    ++rit;
                }
            }
        }
     
        std::set<RegisterAvailability>::iterator it=register_availability.begin();
        while(it!=register_availability.end() && it->cycle<=avail.cycle) //Go through the set from the beginning until all earlier dependences have been eliminated 
        {
            if(it->cycle!=RegisterAvailability::EVENT_DEPENDENT) //If register already available
            {
                //std::cout << "Erasing 2" << *it << "\n";
                it=std::set<RegisterAvailability>::iterator{register_availability.erase(it++)}; //Erase and advance
                //std::cout << "---------Register availability has " << register_availability.size() << " entries\n";
            }
            else
            {
                ++it;
            }
        }
        
        return raw;
    }
    
    void AbstractDispatcher::markRegisterAsUsed(uint8_t reg) 
    {
        if(latency_last_inst_!=1) //If latency is 1 the the raw will always be satisfied by the time is checked (next cycle), do not add
        {
            sparta::Clock::Cycle cycle_ready; 
            if(latency_last_inst_!=RegisterAvailability::EVENT_DEPENDENT)
            {
                cycle_ready=num_dispatched_since_last_ev+getClock()->currentCycle()+latency_last_inst_;
            }
            else
            {
                cycle_ready=RegisterAvailability::EVENT_DEPENDENT;
            }
            RegisterAvailability reg_avail(reg, cycle_ready);


            //std::cout << "Inserting " << reg_avail << "\n";

            register_availability.insert(reg_avail);
        }
    }   
    
    void AbstractDispatcher::markRegisterAsAvailable(uint8_t reg)    
    {
        std::set<RegisterAvailability>::iterator it=register_availability.begin();
        bool found=false;
        if(reg!=0)
        {
            while(it!=register_availability.end() && !found)
            {
                if(it->reg==reg)
                {
                    register_availability.erase(it);
                    found=true;
                }
                ++it;
            }
            if(found && current_stall_==WAITING_ON_MEMORY && reg_expected_for_raw==reg)
            {
                dispatchInstructions_();
            }
        }
    }

}
#endif
