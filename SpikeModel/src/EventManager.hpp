
#ifndef __EVENT_MANAGER_HH__
#define __EVENT_MANAGER_HH__

#include <memory>
#include "ServicedRequests.hpp"
#include "Tile.hpp"
#include "NoCMessage.hpp"
#include "AddressMappingPolicy.hpp"

class SpikeModel; //Forward declaration

namespace spike_model
{
    class Tile; //Forward declaration    

    class EventManager
    {
        /**
         * \class spike_model::EventManager
         *
         * \brief EventManager is the main interface between Spike and Sparta.
         *
         * Instances of SimulationOrchestrator use an instance of EventManager to forward a Request 
         * to Sparta and check for its completion. The EventManager also holds information regarding
         * the memory hiwerarchy of the modelled architecture, to update the data of requests.
         *
         */

        friend class ::SpikeModel;

        public:
 
            /*!
             * \brief Constructor for the request manager
             * \param tiles The tiles handled my the request manager
             * \param cores_per_tile The number of cores per tile
             */
            EventManager(std::vector<Tile *> tiles, uint16_t cores_per_tile);
            
            /*!
             * \brief Forward an L2 request to the memory hierarchy.
             * \param req The request to forward 
             */
            virtual void putRequest(std::shared_ptr<CacheRequest> req);
            
            /*!
             * \brief Notify the completion of request
             * \param req The request that has been completed
             */
            void notifyAck(const std::shared_ptr<CacheRequest>& req);
        

            /*!
             * \brief Query for the availability of serviced requests
             * \return true if there is any serviced request
             */
            bool hasServicedRequest();

            /*!
             * \brief Get a serviced request
             * \return A request that has been serviced
             */
            std::shared_ptr<CacheRequest> getServicedRequest();

        protected:
            std::vector<Tile *> tiles_;
            uint16_t cores_per_tile_;
           
        
        private:
            ServicedRequests serviced_requests_;
            
            
            /*!
             * \brief Set the storage for the requests that have been acknowledged 
             * \param s The storage for acknowledged requests
             * \note This method is called through friending by SpikeModel
             */
            void setServicedRequestsStorage(ServicedRequests& s)
            {
                serviced_requests_=s;
//                std::cout << s.hasRequest();
            }
            
    };
}
#endif
