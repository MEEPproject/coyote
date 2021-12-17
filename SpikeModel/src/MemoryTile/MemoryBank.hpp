
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

namespace spike_model
{
    class MemoryController; //Forward declaration

    class MemoryBank : public sparta::Unit, public LogCapable
    {
        /*!
         * \class spike_model::MemoryBank
         * \brief A MemoryBank implements a simple state machine that models the operations of a memory bank
         * 
         * State changes are triggered by:
         * 1. Instances of BankCommand received the issue function
         * 2. Events modelling the behavior of the bank, such as the time it takes to open a row
         */
        enum class BankState
        {
            OPEN,
            OPENING,
            CLOSED,
            CLOSING,
            READING,
            WRITING,
            ACCESSING
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
                PARAMETER(uint64_t, delay_open, 50, "The delay to open a row")
                PARAMETER(uint64_t, delay_close, 50, "The delay to close a row")
                PARAMETER(uint64_t, delay_read, 20, "The delay to read")
                PARAMETER(uint64_t, delay_write, 20, "The delay to write")
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
            void issue(std::shared_ptr<BankCommand> c);

            /*!
             * \brief Check if the bank is ready for a new command
             * \return True if the current state is OPEN or CLOSED
             */
            bool isReady();
    
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

            //This function needs to be public to be associated to an event, but should not be called otherwise

        private:
            uint64_t num_rows;
            uint64_t num_columns;
            uint64_t column_element_size;
            uint64_t delay_open;
            uint64_t delay_close;
            uint64_t delay_read;
            uint64_t delay_write;

            uint64_t current_row; //TODO: This is not updated!!!
            BankState state=BankState::CLOSED;
            
            MemoryController * mc;
            
            /*!
             * \brief Notify the completion of a command
             * \param c The command that has been completed
             */
            void notifyCompletion(const std::shared_ptr<BankCommand>& c);
        
            sparta::PayloadEvent<std::shared_ptr<BankCommand>, sparta::SchedulingPhase::Tick> command_completed_event {&unit_event_set_, "command_completed_", CREATE_SPARTA_HANDLER_WITH_DATA(MemoryBank, notifyCompletion, std::shared_ptr<BankCommand>)};
            
            sparta::Counter count_activate_=sparta::Counter(getStatisticSet(), "activate", "Number of activats", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_precharge_=sparta::Counter(getStatisticSet(), "precharge", "Number of precharges", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_read_=sparta::Counter(getStatisticSet(), "read", "Number of reads", sparta::Counter::COUNT_NORMAL);
            sparta::Counter count_write_=sparta::Counter(getStatisticSet(), "write", "Number of writes", sparta::Counter::COUNT_NORMAL);
    };
}
#endif
