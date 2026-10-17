#ifndef ARITH_INSTRUCTIONS_HPP
#define ARITH_INSTRUCTIONS_HPP

#include <string>

#include "rvsim/alu.hpp"
#include "rvsim/instruction.hpp"
#include "rvsim/types.hpp"

namespace rvsim {

// class for the R-type instructions
class RegisterOpInstruction final: public Instruction {

    public:
        RegisterOpInstruction(Addr pc, Word word, AluOp _aluOp) :
                                Instruction(pc, word),
                                    aluOp(_aluOp) {
            extract_fields();
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return _disassemble;
        }
        InstructionKind kind() const override {
            return _kind;
        }
        std::string mnemonic() const override {
            return _mnemonic;
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;
        // get the type of operation
        AluOp aluOp;
};

// class for the I-type instructions
class ImmediateOpInstruction final : public Instruction {

    public:
        ImmediateOpInstruction(Addr pc, Word word, AluOp _aluOp) :
                                    Instruction(pc, word),
                                        aluOp(_aluOp) {

            extract_fields();
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return _disassemble;
        }
        InstructionKind kind() const override {
            return _kind;
        }
        std::string mnemonic() const override {
            return _mnemonic;
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;
        // get the type of operation
        AluOp aluOp;
};

// class for the LUI instruction (U-type instruction)
class LuiInstruction final : public Instruction {

    public:
        LuiInstruction(Addr pc, Word word) :
                        Instruction(pc, word) {

            extract_fields();
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return _disassemble;
        }
        InstructionKind kind() const override {
            return _kind;
        }
        std::string mnemonic() const override {
            return _mnemonic;
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;
};

// class for the auipc instruction (U-type instruction)
class AuipcInstruction final : public Instruction {

    public:
        AuipcInstruction(Addr pc, Word word) :
                        Instruction(pc, word) {

            extract_fields();
        }

        //return the human readable instruction
        std::string disassemble() const override {
            return _disassemble;
        }
        InstructionKind kind() const override {
            return _kind;
        }
        std::string mnemonic() const override {
            return _mnemonic;
        }
    private:
        // helper function to extract everything
        void extract_fields() noexcept;

};


}




#endif