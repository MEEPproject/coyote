// <SkeletonSimulation.hpp> -*- C++ -*-

#ifndef __SPARTA_EXAMPLE_H__
#define __SPARTA_EXAMPLE_H__

#include "sparta/app/Simulation.hpp"
#include "sparta/trigger/ExpiringExpressionTrigger.hpp"
#include <cinttypes>

namespace sparta {
    class ParameterSet;
}

namespace minimum_two_phase_example
{
    class SpartaExample : public sparta::app::Simulation
    {
    public:

        SpartaExample(sparta::Scheduler & scheduler);

        virtual ~SpartaExample();

    private:

        void buildTree_() override;

        void configureTree_() override;

        void bindTree_() override;
    };
}

// __SPIKE_MODEL_H__
#endif
