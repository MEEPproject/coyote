#ifndef __MCPU_SetVVL_HH__
#define __MCPU_SetVVL_HH__

#include "EventVisitor.hpp"
#include "VectorElementType.hpp"
#include <iostream>

namespace spike_model
{
    class MCPUSetVVL : public Event, public std::enable_shared_from_this<MCPUSetVVL>
    {
        /**
         * \class spike_model::MCPUSetVVL
         *
         * \brief MCPUSetVVL passes the virtual vector length to MCPU.
         *
         */

        public:

            MCPUSetVVL() = delete;
            MCPUSetVVL(MCPUSetVVL const&) = delete;
            MCPUSetVVL& operator=(MCPUSetVVL const&) = delete;

            /*!
             * \brief Constructor for MCPUSetVVL
             * \param pc The program counter of the instruction that generates the event
             */
            MCPUSetVVL(uint64_t pc):Event(pc) {}

            /*!
             * \brief Constructor for MCPUSetVVL
             * \param VVL The virtual vector length set by scalar core
             * \param pc The program counter of the instruction that finished the simulation
             * \param time The timestamp when the event is submitted to sparta
             * \param c The core which submitted the event
             */
            MCPUSetVVL(uint64_t AVL, size_t regId, uint64_t pc, uint64_t time, uint16_t c): Event(pc, time, c)
                          , AVL(AVL), regId(regId) {
                setLMUL(1);
                set_width(VectorElementType::BIT64);
            }

            /*!
             * \brief Handle the event
             * \param v The visitor to handle the event
             */
            void handle(EventVisitor * v) override {
                v->handle(shared_from_this());
            }

            uint64_t getVVL() {return VVL;}

            void setVVL(uint64_t VVL) {this->VVL = VVL;}

            uint64_t getAVL() {return AVL;}

            void setAVL(uint64_t AVL) {this->AVL = AVL;}

            /*!
             * \brief Set the id and type of the destination register for the SetVVL request
             * \param r The id of the register
             * \param t The type of the register
             */
            void setDestinationReg(size_t r) {regId=r;}

            /*!
             * \brief Get the destination register id for the SetVVL request
             * \return The id of the register
             */
            size_t getDestinationRegId() {return regId;}
            
            /*!
             * \brief Set the LMUL setting for the current VVL request.
             * \param lmul, the lmul setting.
             */
            void setLMUL(uint8_t lmul) {this->lmul = lmul;}
            
            /*!
             * \brief Return the LMUL setting
             * \return The LMUL setting for this request.
             */
            uint8_t getLMUL() {return lmul;}
            
            /*!
             * \brief Set the vector element width.
             * \param The width of the vector elements.
             */
            void set_width(VectorElementType width) {this->width = width;}
            
            /*!
             * \brief Get the vector element width
             * \return The element width of the current VVL request.
             */
            VectorElementType get_width() {return width;}

        private:
            uint8_t lmul;
            VectorElementType width;
            uint64_t AVL, VVL;
            size_t regId;
    };
    
    
    inline std::ostream& operator<<(std::ostream &str, MCPUSetVVL &instr) {
        str << "AVL: " << instr.getAVL() << ", VVL: " << instr.getVVL() << ", coreID: " << instr.getCoreId();
        return str;
    }
}
#endif
