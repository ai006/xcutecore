
#include <format>

#include "rvsim/control_instructions.hpp"


namespace rvsim {

void
JalInstruction::extract_fields() noexcept{

    // set the instructions characteristics
    _writes_rd = true;
    _is_control_flow = true;

    _opcode = rvsim::opcode(_word);

    // get the source and destination registers
    _rd = rvsim::rd(_word);

    _imm = rvsim::imm_j(_word);

    // instruction kind 
    _kind = InstructionKind::Jal;
    _mnemonic = "jal";

    _disassemble = std::format("{}  {}, {}", _mnemonic,
                                            rvsim::abi_name(_rd),
                                            _imm);


}

void
JalrInstruction::extract_fields() noexcept{

    // set the instructions characteristics
    _writes_rd = true;
    _reads_rs1 = true;
    _is_control_flow = true;

    _opcode = rvsim::opcode(_word);

    // get the source and destination registers
    _rd = rvsim::rd(_word);
    _rs1 = rvsim::rs1(_word);

    _imm = rvsim::imm_i(_word);

    // instruction kind 
    _kind = InstructionKind::Jalr;
    _mnemonic = "jalr";

    _disassemble = std::format("{}  {}, {}, {}", _mnemonic,
                                            rvsim::abi_name(_rd),
                                            rvsim::abi_name(_rs1),
                                            _imm);


}


}