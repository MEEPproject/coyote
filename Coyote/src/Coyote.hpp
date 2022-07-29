// 
// Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
//                Supercomputación
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the LICENSE file in the root directory of the project for the
// specific language governing permissions and limitations under the
// License.
// 

// <SkeletonSimulation.hpp> -*- C++ -*-

#ifndef __COYOTE_H__
#define __COYOTE_H__

#include "spike_wrapper.h"

#include "sparta/app/Simulation.hpp"
#include "sparta/trigger/ExpiringExpressionTrigger.hpp"
#include <cinttypes>
#include "Request.hpp"
#include "ServicedRequests.hpp"
#include "FullSystemSimulationEventManager.hpp"
#include "L2SharingPolicy.hpp"
#include "AddressMappingPolicy.hpp"
#include "Logger.hpp"

namespace sparta {
    class Baz;
    class ParameterSet;
}
namespace sparta_simdb {
    class DatabaseTester;
}

namespace coyote{ class CPUFactory;}

/*!
 * \brief Coyote which builds the model and configures it
 */
class Coyote : public sparta::app::Simulation
{
public:

    /*!
     * \brief Construct Coyote
     * \param be_noisy Be verbose -- not necessary, just an skeleton
     */
    Coyote(sparta::Scheduler & scheduler);

    // Tear it down
    virtual ~Coyote();

    std::shared_ptr<coyote::FullSystemSimulationEventManager> createRequestManager();

    coyote::Logger * getLogger();

    double getAvgArbiterLatency();
    double getAvgL2Latency();
    double getAvgNoCLatency();
    double getAvgLLCLatency();
    double getAvgMemoryControllerLatency();

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


    /*!
     * \brief Get the factory for topology build
     */
    auto getCPUFactory_() -> coyote::CPUFactory*;

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

// __COYOTE_H__
#endif
