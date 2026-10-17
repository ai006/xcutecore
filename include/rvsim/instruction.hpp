#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <string>

#include "rvsim/types.hpp"

namespace rvsim {

//the kind of instruction
enum class InstructionKind {
    RegisterOp,
    ImmediateOp,
    Lui,
    Auipc,
    Load,
    Store,
    Branch,
    Jal,
    Jalr,
    System,
    Fence,
    Illegal
};


class Instruction {
    
    protected:
        Instruction(Addr pc, Word word) :
                        _pc(pc), _word(word) {

        }
    
    public:
        virtual ~Instruction() = default;
        // return human readable instruction
        virtual std::string disassemble() const = 0;
        
        // return the kind of instruction
        virtual InstructionKind kind() const = 0;

        virtual std::string mnemonic() const = 0;

        // characteristics of the instructions
        // *****    start   ****
        virtual bool reads_rs1() const { return _reads_rs1; }
        virtual bool reads_rs2() const { return _reads_rs2; }
        virtual bool writes_rd() const { return _writes_rd; }
        virtual bool is_load() const { return _is_load; }
        virtual bool is_control_flow() const{ return _is_control_flow; }
        virtual bool is_system() const { return  _is_system; }
        // *****    end   ****

        // getters for src, dest, and imm
        Addr pc() const { return _pc; }
        Word word() const { return _word; }
        Word imm() const { return _imm; }
        RegIndex rd() const { return _rd; }
        RegIndex rs1() const { return _rs1; }
        RegIndex rs2() const { return _rs2; }

    protected:
        Addr _pc = 0;
        Word _word = 0;

        // booleans to indicate what the
        // instruction does
        bool _reads_rs1 = false;
        bool _reads_rs2 = false;
        bool _writes_rd = false;
        bool _is_load = false;
        bool _is_control_flow = false;
        bool _is_system = false;

        Word _imm = 0;
        RegIndex _rs1 = 0;
        RegIndex _rs2 = 0;
        RegIndex _rd = 0;
        Word _opcode = 0;
        
        //each set the default as illegal until decode
        InstructionKind _kind = InstructionKind::Illegal;

        std::string _mnemonic = "";
        // the instruction in string format
        std::string _disassemble = "";

};




}

#endif