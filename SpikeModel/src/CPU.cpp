// <CPU.cpp> -*- C++ -*-


#include "CPU.hpp"

//! \brief Name of this resource. Required by sparta::UnitFactory
constexpr char spike_model::CPU::name[];

//! \brief Constructor of this CPU Unit
spike_model::CPU::CPU(sparta::TreeNode* node, const spike_model::CPU::CPUParameterSet* params) :
    sparta::Unit{node},
    frequency_ghz_{params->frequency_ghz}
{
}

//! \brief Destructor of this CPU Unit
spike_model::CPU::~CPU() = default;
