
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
            PARAMETER(uint16_t, num_l2_banks, 1, "The number of l2 banks")
            PARAMETER(std::string, data_mapping_policy, "set_interleaving", "The data mapping policy")
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

        void setL2BankInfo(uint64_t size, uint64_t assoc, uint64_t line_size)
        {
            l2_bank_size_kbs=size;
            l2_assoc=assoc;
            l2_line_size=line_size;

            block_offset_bits=(uint8_t)ceil(log2(l2_line_size));
            bank_bits=(uint8_t)ceil(log2(in_ports_l2_.size()));

            uint64_t total_l2_size=l2_bank_size_kbs*in_ports_l2_.size()*1024;
            uint64_t num_sets=total_l2_size/(l2_assoc*l2_line_size);
            set_bits=(uint8_t)ceil(log2(num_sets));
            tag_bits=64-(set_bits+block_offset_bits);
            std::cout << "There are " << unsigned(bank_bits) << " bits for banks and " << unsigned(set_bits) << " bits for sets\n";
        }

    private:

        uint16_t num_cores_;
        uint16_t num_l2_banks_;

        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>>> in_ports_l2_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>>> out_ports_l2_;
        std::vector<std::unique_ptr<sparta::DataInPort<std::shared_ptr<L2Request>>>> in_ports_cores_;
        std::vector<std::unique_ptr<sparta::DataOutPort<std::shared_ptr<L2Request>>>> out_ports_cores_;

        std::vector<Core *> cores_;

        uint64_t l2_bank_size_kbs;
        uint64_t l2_assoc;
        uint64_t l2_line_size;

        uint8_t block_offset_bits;
        uint8_t bank_bits;
        uint8_t set_bits; 
        uint8_t bank_displacement;
        uint8_t tag_bits;
        
        enum class MappingPolicy
        {
            SET_INTERLEAVING,
            PAGE_TO_BANK,
        };

        MappingPolicy data_mapping_policy_;

        uint16_t getDestination(std::shared_ptr<L2Request> req);
    };
}
#endif
