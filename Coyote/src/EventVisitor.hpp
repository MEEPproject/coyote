#ifndef __EVENT_VISITOR_HH__
#define __EVENT_VISITOR_HH__

#include <cinttypes>
#include <memory>
#include <stdio.h>

namespace coyote
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
    class VectorWaitingForScalarStore;

    class EventVisitor
    {

        /**
         * \class coyote::EventVisitor
         *
         * \brief EventVisitor uses the Visitor design pattern to operate on Events
         *
         * ExecutionDrivenSimulationOrchestrator inherits from this class. Thiss efectively separates all Sparta files from the Spike side compilation.
         * Each class extending EventVisitor will potentially define its own handle methods for particular kinds of events.
         *
         */
        
        public:

            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::Event> e);
            
            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::CoreEvent> e);
            
            /*!
             * \brief Handles an event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::RegisterEvent> e);
            
            /*!
             * \brief Handles a sync event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::Sync> e);
            
            /*!
             * \brief Handles a sync event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::VectorWaitingForScalarStore> e);

            /*!
             * \brief Handles a finish event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::Finish> e);
            
            /*!
             * \brief Handles a fence event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::Fence> e);
            
            /*!
             * \brief Handles a request event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::Request> e);
            
            /*!
             * \brief Handles a cache requst event
             * \param e The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::CacheRequest> e);

            /*!
             * \brief Handles a scratchpad request request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::ScratchpadRequest> r);
            
            /*!
             * \brief Handles a MCPU request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::MCPUSetVVL> e);
            
            /*!
             * \brief Handles a Instruction latency event request
             * \param r The event to handle
             */
            virtual void handle(std::shared_ptr<coyote::InsnLatencyEvent> e);

            /*!
             * \brief Handles an MCPUInstruction
             * \param i The instruction to handle
             */
            virtual void handle(std::shared_ptr<coyote::MCPUInstruction> i);
    };
}
#endif