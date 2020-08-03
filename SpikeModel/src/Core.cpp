#include "Core.hpp"

#include "sparta/ports/DataPort.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/events/SingleCycleUniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/log/MessageSource.hpp"
#include "sparta/statistics/Counter.hpp"

#include "L2Request.hpp"
#include <chrono>

namespace spike_model
{
    const char Core::name[] = "core";
    // Constructor
    Core::Core(sparta::TreeNode * node,
                       const CoreParameterSet * p) :
        sparta::Unit(node),
        finished_(false),
        latency_to_l2_(p->latency_to_l2)
    {

        in_port_.registerConsumerHandler
                (CREATE_SPARTA_HANDLER_WITH_DATA(Core, ack_, std::shared_ptr<L2Request>));
        
        sparta::StartupEvent(node, CREATE_SPARTA_HANDLER(Core, startup_));
    }

    void Core::startup_()
    {
        running_=true;
        getMisses_();
    }

    void Core::getMisses_()
    {
        if(!finished_)
        {
            std::list<std::shared_ptr<spike_model::L2Request>> new_misses;
            bool success=spike->simulateOne(id_, getClock()->currentCycle(), new_misses);

            //FETCH MISSES ARE SERVICED WHETHER THERE IS A RAW OR NOT
            if(new_misses.size()>0 && new_misses.front()->getType()==L2Request::AccessType::FETCH)
            {
                handleMiss_(new_misses.front());
                new_misses.pop_front();
                running_=false;
            }

            //IF NO RAW AND NO FETCH MISS
            if(success && running_)
            {
                count_simulated_instructions_++;
                if(new_misses.size()>0)
                {
                    for(std::shared_ptr<spike_model::L2Request> miss: new_misses)
                    {
                        if(miss->getType()!=L2Request::AccessType::WRITEBACK)
                        {
                            handleMiss_(miss);
                        }
                        else //WRITEBACKS ARE HANDLED LAST
                        {
                            pending_writebacks_.push_back(miss);
                        }
                    }
                }

            }
            else
            {
                if(running_)
                {
                    count_dependency_stalls_++;
                    running_=false;
                }
                else
                {
                    fetch_stalls_++;
                }
                for(std::shared_ptr<spike_model::L2Request> miss: new_misses)
                {   
                    pending_misses_.push_back(miss); //Instructions are not replayed, so we have to store the misses of a raw or under a fetch, so they are serviced later
                }
            }

        }
        else
        {
            //printf("Finished\n");
            //std::cout << "Simulating in spike took " << d << "nanoseconds\n";
        }
        
        if(!finished_ && running_)
        {
            simulate_inst_event_.schedule(sparta::Clock::Cycle(1));
        }
    }
    
    void Core::handleMiss_(std::shared_ptr<spike_model::L2Request> miss)
    {
        if(miss->getType()==L2Request::AccessType::FINISH)
        {
            finished_=true;
        }

        if(!finished_)
        {
            count_l2_requests_++;
            out_port_.send(miss);
        }

    }
    
    void Core::ack_(const std::shared_ptr<L2Request> & req)
    {
        if(pending_misses_.size()>0) //If this was a fetch or there was a raw and there are data misses for the instructions
        {
            while(pending_misses_.size()>0)
            {
                std::shared_ptr<spike_model::L2Request> miss=pending_misses_.front();
                pending_misses_.pop_front();
                handleMiss_(miss);
            }
        }
        else
        {
            while(pending_writebacks_.size()>0) //Writebacks are sent last
            {
                std::shared_ptr<spike_model::L2Request> miss=pending_writebacks_.front();
                pending_writebacks_.pop_front();
                handleMiss_(miss);
            }
        }

        bool can_run=true;
        bool is_load=req->getType()==L2Request::AccessType::LOAD;

        if(is_load)
        {
            //What about vectors??? More than one miss might be necessary to free a single register
            can_run=spike->ackRegister(req, getClock()->currentCycle());
        }

        if(!running_ && can_run)
        {
            running_=true;
            simulate_inst_event_.schedule(sparta::Clock::Cycle(2)); //We schedule in 2 cycles because the rawing instruction is not actually replayed
        }
    }
}
