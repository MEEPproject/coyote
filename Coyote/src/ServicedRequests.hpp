
#ifndef __SERVICED_REQUESTS_HH__
#define __SERVICED_REQUESTS_HH__

#include <memory>

#include "Event.hpp"
#include <queue>

namespace coyote
{

    class ServicedRequests
    {
        /**
         * \class coyote::ServicedRequests
         *
         * \brief Stores instances of Request that have been already been serviced.
         *
         * In general, FullSystemSimulationEventManager adds requests, while the ExecutionDrivenSimulationOrchestrator gets them.
         *
         */
        public:

            /*!
             * \brief Constructor
             */
            ServicedRequests()
            {
                serviced_misses=std::make_shared<std::queue<std::shared_ptr<Event>>> ();
            }

            /*!
             * \brief Adds a request that has been serviced
             * \param req The request that has been serviced
             */
            void addRequest(std::shared_ptr<Event> req)
            {
                serviced_misses->push(req);
            }

            /*!
             * \brief Get a serviced request
             * \return The Request
             */
            std::shared_ptr<Event> getRequest()
            {
                std::shared_ptr<Event> res=serviced_misses->front();
                serviced_misses->pop();
                return res;
            }

            /*!
             * \brief Check if there is any serviced request available
             * \return true if there is a request
             */
            bool hasRequest()
            {
                return !serviced_misses->empty();
            }

        private:
           std::shared_ptr<std::queue<std::shared_ptr<Event>>> serviced_misses;
    };
}
#endif
