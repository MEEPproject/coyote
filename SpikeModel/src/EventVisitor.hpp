#ifndef __EVENT_VISITOR_HH__
#define __EVENT_VISITOR_HH__

#include <cinttypes>
#include <memory>
#include <stdio.h>

namespace spike_model
{
    class Event;
    class CoreEvent;
    class RegisterEvent;
    class Sync;
    class Request;
    class CacheRequest;
    class ScratchpadRequest;
    class Fence;
    class Finish;
    class MCPUSetVVL;
    class MCPUInstruction;
    class InsnLatencyEvent;

    class EventVisitor
    {

        /**
         * \class spike_model::EventVisitor
         *
         * \brief EventVisitor uses the Visitor design pattern to operate on Events
         *
         * SimulationOrchestrator inherits from this class. Thiss efectively separates all Sparta files from the Spike side compilation.
         * Each class extending EventVisitor will potentially define its own handle methods for particular kinds of events.
         *
         */
        
        public:

            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Event> e);
            
            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::CoreEvent> e);
            
            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::RegisterEvent> e);
            
            /*!
             * \brief Handles a sync event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Sync> e);

            /*!
             * \brief Handles a finish event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Finish> e);
            
            /*!
             * \brief Handles a fence event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Fence> e);
            
            /*!
             * \brief Handles a request event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::Request> e);
            
            /*!
             * \brief Handles a cache requst event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::CacheRequest> e);

            /*!
             * \brief Handles a scratchpad request request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::ScratchpadRequest> r);
            
            /*!
             * \brief Handles a MCPU request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::MCPUSetVVL> e);
            
            /*!
             * \brief Handles a Instruction latency event request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<spike_model::InsnLatencyEvent> e);

            /*!
             * \brief Handles an MCPUInstruction
             * \param i The instruction to handle
             */
            virtual void handle(std::shared_ptr<spike_model::MCPUInstruction> i);
    };
}
#endif
