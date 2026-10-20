
#include <format>

#include "rvsim/fields.hpp"
#include "rvsim/system_instructions.hpp"

namespace rvsim {

void
SystemInstruction::extract_fields() noexcept{

    // set the instructions characteristics
    _is_system = true;

    _opcode = rvsim::opcode(_word);

    _imm = rvsim::imm_i(_word);


    switch(_imm) {
        case 0 :
            _mnemonic = "ecall";
            break;
        case 1 :
            _mnemonic = "ebreak";
    }

    // instruction kind 
    _kind = InstructionKind::System;

    _disassemble = std::format("{}", _mnemonic);
}

void
FenceInstruction::extract_fields() noexcept{

    _opcode = rvsim::opcode(_word);

    // hold the fm
    _imm = rvsim::imm_i(_word);

    // instruction kind 
    _kind = InstructionKind::Fence;
    _mnemonic = "fence";

    _disassemble = std::format("{}", _mnemonic);
}

void
IllegalInstruction::extract_fields() noexcept{

    // instruction kind 
    _kind = InstructionKind::Illegal;
    _mnemonic = "illegal";

    _disassemble = std::format("{}", _mnemonic);


}

}