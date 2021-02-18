// <CPU.h> -*- C++ -*-


#ifndef __CPU_H__
#define __CPU_H__

#include <string>

#include "sparta/simulation/Unit.hpp"
#include "sparta/simulation/TreeNode.hpp"
#include "sparta/simulation/ParameterSet.hpp"

namespace spike_model{

/**
 * @file  CPU.h
 * @brief CPU Unit acts as a logical unit containing multiple cores
 *
 * CPU unit will
 * 1. Attach itself to the root Simulation Device node
 * 2. Use its factory to create multiple cores
 * 3. Use sub-factories within its factory to create microarchitecture units
 */
class CPU : public sparta::Unit{
public:

    //! \brief Parameters for CPU model
    class CPUParameterSet : public sparta::ParameterSet{
    public:
        CPUParameterSet(sparta::TreeNode* n) : sparta::ParameterSet(n){}

        // Dummy configuration parameters and environment variables that affect CPU utilization
        PARAMETER(double, frequency_ghz, 1.2, "CPU Clock frequency")
    };

    //! \brief Name of this resource. Required by sparta::UnitFactory
    static constexpr char name[] = "cpu";

    /**
     * @brief Constructor for CPU
     *
     * @param node The node that represents (has a pointer to) the CPU
     * @param p The CPU's parameter set
     */
    CPU(sparta::TreeNode* node, const CPUParameterSet* params);

    //! \brief Destructor of the CPU Unit
    ~CPU();
private:

    //! \brief Internal configuration units of this processor
    double frequency_ghz_;
}; // class CPU
}  // namespace spike_model
#endif
