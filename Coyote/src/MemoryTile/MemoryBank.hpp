
#ifndef __MEMORY_BANK_H__
#define __MEMORY_BANK_H__

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

#include "LogCapable.hpp"
#include "BankCommand.hpp"
#include "MemoryController.hpp"

namespace coyote
{
    class MemoryController; //Forward declaration

    class MemoryBank : public sparta::Unit, public LogCapable
    {
        /*!
         * \class coyote::MemoryBank
         * \brief A MemoryBank implements a simple state machine that models the operations of a memory bank
         * 
         * State changes are triggered by:
         * 1. Instances of BankCommand received the issue function
         * 2. Events modelling the behavior of the bank, such as the time it takes to open a row
         */
        enum class BankState
        {
            OPEN,
            CLOSED
        };

        public:
            /*!
             * \class MemoryBankParameterSet
             * \brief Parameters for MemoryBank model
             */
            class MemoryBankParameterSet : public sparta::ParameterSet
            {
            public:
                //! Constructor for MemoryBankParameterSet
                MemoryBankParameterSet(sparta::TreeNode* n):
                    sparta::ParameterSet(n)
                {
                }
                PARAMETER(uint64_t, num_rows, 65536, "The number of rows")
                PARAMETER(uint64_t, num_columns, 1024, "The number of columns")
                PARAMETER(uint64_t, column_element_size, 8, "The size of column elements")
                PARAMETER(uint8_t, burst_length, 2, "The number of columns handled back to back as a result of a READ/WRITE")
            };

            /*!
             * \brief Constructor for MemoryBank
             * \note  node parameter is the node that represent the MemoryBank and
             *        p is the MemoryBank parameter set
             */
            MemoryBank(sparta::TreeNode* node, const MemoryBankParameterSet* p);

            ~MemoryBank() {
                debug_logger_ << getContainer()->getLocation()
                              << ": "
                              << std::endl;
            }

            //! name of this resource.
            static const char name[];

        public:

            /*!
             * \brief Issue a command. This will potentially trigger a state change
             * \param c The command to issue
             */
            void issue(const std::shared_ptr<BankCommand>& c);

            /*!
             * \brief Check if a row is currently open
             * \return True if the state is OPEN
             */
            bool isOpen();
            
            /*!
             * \brief Get the row currently open 
             * \return The row that is open
             */
            uint64_t getOpenRow();
 
            /*!
             * \brief Set the memory controller that is in charge if this bank
             * \param controller The controller
             */
            void setMemoryController(MemoryController * controller);

            /*
             * \brief Sets the memory spec latencies
             * \param spec The latencies
             */
            void setMemSpec(std::shared_ptr<std::vector<uint64_t>> spec);
            
            /*!
             * \brief Get the number of rows in the bank
             * \return The number of rows
             */
            uint64_t getNumRows();
            
            /*!
             * \brief Get the number of columns in a row
             * \return The number of columns
             */
            uint64_t getNumColumns();


            /*!
             * \brief Get the burst size in bytes
             * \return The burst size in bytes
             */
            uint64_t getBurstSize();
            
            /*!
             * \brief Get the burst length
             * \return The burst size in bytes
             */
            uint8_t getBurstLength();
            

        private:
            uint64_t num_rows;
            uint64_t num_columns;
            uint64_t column_element_size;
            uint8_t burst_length;

            uint64_t current_row; //TODO: This is not updated!!!
            BankState state=BankState::CLOSED;
            
            MemoryController * mc;

            std::shared_ptr<std::vector<uint64_t>> latencies;

            /*!
             * \brief Notify the completion of a command
             * \param c The command that has been completed
             */
            void notifyDataAvailable(const std::shared_ptr<BankCommand>& c);
    
            /*
             * \brief Notify that a DRAM timing requirement has been met
             */
            void notifyTimingEvent();
       
            sparta::PayloadEvent<std::shared_ptr<BankCommand>, sparta::SchedulingPhase::Tick> data_available_event {&unit_event_set_, "data_available_", CREATE_SPARTA_HANDLER_WITH_DATA(MemoryBank, notifyDataAvailable, std::shared_ptr<BankCommand>)};
            
            sparta::UniqueEvent<sparta::SchedulingPhase::Tick> timing_event {&unit_event_set_, "notify_timing_", CREATE_SPARTA_HANDLER(MemoryBank, notifyTimingEvent)};
			
            sparta::Counter count_activate_=sparta::Counter(getStatisticSet(), "activate", "Number of activats", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_precharge_=sparta::Counter(getStatisticSet(), "precharge", "Number of precharges", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_read_=sparta::Counter(getStatisticSet(), "read", "Number of reads", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_write_=sparta::Counter(getStatisticSet(), "write", "Number of writes", sparta::Counter::COUNT_NORMAL);
    };
}
#endif
