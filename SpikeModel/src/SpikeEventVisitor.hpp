#ifndef __SPIKE_EVENT_VISITOR_HH__
#define __SPIKE_EVENT_VISITOR_HH__

#include <cinttypes>
#include <memory>
#include <stdio.h>

namespace spike_model
{
    class SpikeEvent; //Forward declaration
    class Request;
    class Finish;
    class Fence;

    class SpikeEventVisitor
    {
        /**
         * \class spike_model::SpikeEventVisitor
         *
         * \brief SpikeEventVisitor uses the Visitor design pattern to operate on SpikeEvents
         *
         * SimulationOrchestrator inherits from this class. Thiss efectively separates all Sparta files from the Spike side compilation.
         * Each class extending SpikeEventVisitor will potentially define its own handle methods for particular kinds of events.
         *
         */
        
        public:


            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Request> e)=0;
            
            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Finish> e)=0;
            
            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Fence> e)=0;
            
    };
}
#endif
