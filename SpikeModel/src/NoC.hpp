
#ifndef __NoC_H__
#define __NoC_H__

#include "sparta/ports/PortSet.hpp"
#include "sparta/ports/SignalPort.hpp"
#include "sparta/ports/DataPort.hpp"
#include "sparta/events/EventSet.hpp"
#include "sparta/events/UniqueEvent.hpp"
#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/ParameterSet.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/collection/Collectable.hpp"
#include "sparta/events/StartupEvent.hpp"
#include "sparta/resources/Pipeline.hpp"
#include "sparta/resources/Buffer.hpp"
#include "sparta/pairs/SpartaKeyPairs.hpp"
#include "sparta/simulation/State.hpp"
#include "sparta/utils/SpartaSharedPointer.hpp"

#include <memory>

#include "L2Request.hpp"
#include "Core.hpp"


namespace spike_model
{
    class Core; //Forward declaration    

    class NoC : public sparta::Unit
    {
    public:
        /*!
         * \class NoCParameterSet
         * \brief Parameters for NoC model
         */
        class NoCParameterSet : public sparta::ParameterSet
        {
        public:
            //! Constructor for NoCParameterSet
            NoCParameterSet(sparta::TreeNode* n):
                sparta::ParameterSet(n)
            {
            }
            PARAMETER(uint16_t, num_cores, 1, "The number of cores")
        };

        /*!
         * \brief Constructor for NoC
         * \note  node parameter is the node that represent the NoC and
         *        p is the NoC parameter set
         */
        NoC(sparta::TreeNode* node, const NoCParameterSet* p);

        ~NoC() {
            debug_logger_ << getContainer()->getLocation()
                          << ": "
                          << std::endl;
        }

        //! name of this resource.
        static const char name[];


        ////////////////////////////////////////////////////////////////////////////////
        // Type Name/Alias Declaration
        ////////////////////////////////////////////////////////////////////////////////

        void send_(const std::shared_ptr<L2Request> & req);
        void issueAck_(const std::shared_ptr<L2Request> & req);

        void setOrchestrator(unsigned i, Core& o)
        {
            sparta_assert(i < num_cores_);
            cores_[i]=&o;
        }

    private:

        uint16_t num_cores_;

        ////////////////////////////////////////////////////////////////////////////////
        // Input Ports
        ////////////////////////////////////////////////////////////////////////////////

        //Forced to use unique_ptr because ports have no = operator

        sparta::DataInPort<std::shared_ptr<L2Request>> in_l2_ack_
            {&unit_port_set_, "in_l2_ack"};


        ////////////////////////////////////////////////////////////////////////////////
        // Output Ports
        ////////////////////////////////////////////////////////////////////////////////
        

        sparta::DataOutPort<std::shared_ptr<L2Request>> out_l2_req_
            {&unit_port_set_, "out_l2_req"};

        std::vector<Core *> cores_;
    };
}
#endif
