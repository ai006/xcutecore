
#include <format>

#include "rvsim/alu.hpp"
#include "rvsim/arith_instructions.hpp"
#include "rvsim/fields.hpp"
#include "rvsim/register_file.hpp"

namespace rvsim{


// used to extract all the fields required to execute the
// instruction
void
RegisterOpInstruction::extract_fields() noexcept{

    // set the instructions characteristics
    _reads_rs1 = true;
    _reads_rs2 = true;
    _writes_rd = true;

    // get the opcode
    _opcode = rvsim::opcode(_word);

    // get the source and destination registers
    _rs1 = rvsim::rs1(_word);
    _rs2 = rvsim::rs2(_word);
    _rd = rvsim::rd(_word);

    // instruction kind 
    _kind = InstructionKind::RegisterOp;
    _mnemonic = extract_mnemonic(aluOp);

    _disassemble = std::format("{} , {}, {}, {}", _mnemonic,
                                                rvsim::abi_name(_rd),
                                                rvsim::abi_name(_rs1),
                                                rvsim::abi_name(_rs2));
}

void
ImmediateOpInstruction::extract_fields() noexcept {

    // set the instruction flags
    _reads_rs1 = true;
    _writes_rd = true;

    // get the opcode
    _opcode = rvsim::opcode(_word);

    // get the source and destination registers
    _rs1 = rvsim::rs1(_word);
    _rd = rvsim::rd(_word);

    _imm = rvsim::imm_i(_word);

    if(aluOp == AluOp::SLL || aluOp == AluOp::SRL || aluOp == AluOp::SRA)
        _imm = rvsim::bits(_imm, 4, 0);


    // instruction kind 
    _kind = InstructionKind::ImmediateOp;
    _mnemonic = extract_mnemonic_imm(aluOp);

    _disassemble = std::format("{} , {}, {}, {}", _mnemonic,
                                                rvsim::abi_name(_rd),
                                                rvsim::abi_name(_rs1),
                                                _imm);
}

void
LuiInstruction::extract_fields() noexcept{
    // set the instruction flags
    _writes_rd = true;

        // get the opcode
    _opcode = rvsim::opcode(_word);

    // get the source and destination registers
    _rd = rvsim::rd(_word);
    _imm = rvsim::imm_u(_word);

    // instruction kind 
    _kind = InstructionKind::Lui;
    _mnemonic = "lui";
    _disassemble = std::format("{} , {}, {}, {}", _mnemonic,
                                                rvsim::abi_name(_rd),
                                                rvsim::abi_name(_rs1),
                                                _imm);
}

void
AuipcInstruction::extract_fields() noexcept{
    // set the instruction flags
    _writes_rd = true;

        // get the opcode
    _opcode = rvsim::opcode(_word);

    // get the source and destination registers
    _rd = rvsim::rd(_word);
    _imm = rvsim::imm_u(_word);

    // instruction kind 
    _kind = InstructionKind::Auipc;
    _mnemonic = "auipc";

    _disassemble = std::format("{} , {}, {}, {}", _mnemonic,
                                                rvsim::abi_name(_rd),
                                                rvsim::abi_name(_rs1),
                                                _imm);
}


}