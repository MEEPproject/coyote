#ifndef __EVENT_VISITOR_HH__
#define __EVENT_VISITOR_HH__

#include <cinttypes>
#include <memory>
#include <stdio.h>

namespace spike_model
{
    class Event;
    class Sync;
    class Request;
    class CacheRequest;
    class Fence;
    class Finish;
    class MCPURequest;

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

            virtual void handle(std::shared_ptr<spike_model::MCPURequest> e);
    };
}
#endif
