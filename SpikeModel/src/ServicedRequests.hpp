
#ifndef __SERVICED_REQUESTS_HH__
#define __SERVICED_REQUESTS_HH__

#include <memory>

#include "Request.hpp"
#include <queue>

namespace spike_model
{

    class ServicedRequests
    {
        public:

            ServicedRequests()
            {
                serviced_misses=std::make_shared<std::queue<std::shared_ptr<Request>>> ();
            }

            void addRequest(std::shared_ptr<Request> req)
            {
                serviced_misses->push(req);
            }

            std::shared_ptr<Request> getRequest()
            {
                std::shared_ptr<Request> res=serviced_misses->front();
                serviced_misses->pop();
                return res;
            }

            bool hasRequest()
            {
                return !serviced_misses->empty();
            }

        private:
           std::shared_ptr<std::queue<std::shared_ptr<Request>>> serviced_misses;
    };
}
#endif
