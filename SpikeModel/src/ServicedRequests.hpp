
#ifndef __SERVICED_REQUESTS_HH__
#define __SERVICED_REQUESTS_HH__

#include <memory>

#include "Request.hpp"
#include <queue>

namespace spike_model
{

    class ServicedRequests
    {
        /**
         * \class spike_model::ServicedRequests
         *
         * \brief Stores instances of Request that have been already been serviced.
         *
         * In general, RequestManagerIF adds requests, while the SimulationOrchestrator gets them.
         *
         */
        public:

            /*!
             * \brief Constructor
             */
            ServicedRequests()
            {
                serviced_misses=std::make_shared<std::queue<std::shared_ptr<CacheRequest>>> ();
            }

            /*!
             * \brief Adds a request that has been serviced
             * \param req The request that has been serviced
             */
            void addRequest(std::shared_ptr<CacheRequest> req)
            {
                serviced_misses->push(req);
            }

            /*!
             * \brief Get a serviced request
             * \return The Request
             */
            std::shared_ptr<CacheRequest> getRequest()
            {
                std::shared_ptr<CacheRequest> res=serviced_misses->front();
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
           std::shared_ptr<std::queue<std::shared_ptr<CacheRequest>>> serviced_misses;
    };
}
#endif
