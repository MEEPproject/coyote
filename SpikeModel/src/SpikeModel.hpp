// <SkeletonSimulation.hpp> -*- C++ -*-

#ifndef __SPIKE_MODEL_H__
#define __SPIKE_MODEL_H__

#include "spike_wrapper.h"

#include "sparta/app/Simulation.hpp"
#include "sparta/trigger/ExpiringExpressionTrigger.hpp"
#include <cinttypes>


namespace sparta {
    class Baz;
    class ParameterSet;
}
namespace sparta_simdb {
    class DatabaseTester;
}

namespace spike_model{ class CPUFactory; } 
    
/*!
 * \brief SpikeModel which builds the model and configures it
 */
class SpikeModel : public sparta::app::Simulation
{
public:

    /*!
     * \brief Construct SpikeModel
     * \param be_noisy Be verbose -- not necessary, just an skeleton
     */
    SpikeModel(const std::string& topology, sparta::Scheduler & scheduler, uint32_t num_cores, uint32_t num_l2_banks, std::string cmd, std::string isa, bool show_factories);

    // Tear it down
    virtual ~SpikeModel();

    //virtual void run(uint64_t run_time) override;

private:

    //////////////////////////////////////////////////////////////////////
    // Setup

    //! Build the tree with tree nodes, but does not instantiate the
    //! unit yet
    void buildTree_() override;

    //! Configure the tree and apply any last minute parameter changes
    void configureTree_() override;

    //! The tree is now configured, built, and instantiated.  We need
    //! to bind things together.
    void bindTree_() override;

    //! Name of the topology to build
    std::string cpu_topology_;

    //! Number of cores in this simulator. Temporary startup option
    const uint32_t num_cores_;
    const uint32_t num_l2_banks_;
    std::string cmd_;
    std::string isa_;
    
    /*! 
 
     * \brief Get the factory for topology build 
     */ 
    auto getCPUFactory_() -> spike_model::CPUFactory*; 

    bool show_factories_;

    /*!
     * \brief Notification source and dedicated warmup listeners used to mimic
     * legacy report start events.
     */
    std::unique_ptr<sparta::NotificationSource<uint64_t>> legacy_warmup_report_starter_;
    std::vector<std::unique_ptr<sparta::trigger::ExpressionTrigger>> core_warmup_listeners_;
    uint32_t num_cores_still_warming_up_ = 0;
    void onLegacyWarmupNotification_();

    /*!
     * \brief An "on triggered" callback for testing purposes
     */
    void onTriggered_(const std::string & msg);
    bool on_triggered_notifier_registered_ = false;

    /*!
     * \brief If present, test tree node extensions
     */
    void validateTreeNodeExtensions_();
    
};

// __SPIKE_MODEL_H__
#endif
