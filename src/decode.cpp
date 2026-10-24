#ifndef DECODE_HPP
#define DECODE_HPP

#include <memory>

#include "rvsim/arith_instructions.hpp"
#include "rvsim/control_instructions.hpp"
#include "rvsim/fields.hpp"
#include "rvsim/instruction.hpp"
#include "rvsim/load_store_instructions.hpp"
#include "rvsim/system_instructions.hpp"

namespace rvsim {


// function used to decode instructions 
// using the opcode and func3 and func7
std::unique_ptr<Instruction> decode(Word word, Addr pc) {

    uint32_t op = rvsim::opcode(word);
    uint32_t funct3 = rvsim::funct3(word);
    uint32_t funct7 = rvsim::funct7(word);

    // the decoded instruction to return 
    std::unique_ptr<Instruction> inst = nullptr;

    // change word to be an enum class 
    const auto opcode = static_cast<rvsim::Opcode>(op);

    // first we will choose the instruction via opcode
    switch(opcode) {
        case Opcode::LUI :
            inst = std::make_unique<LuiInstruction>(pc, word);
            break;
        case Opcode::AUIPC :
            inst = std::make_unique<AuipcInstruction>(pc, word);
            break;
        case Opcode::JAL :
            inst = std::make_unique<JalInstruction>(pc, word);
            break;
        case Opcode::JALR :
            switch(funct3) {
                case 0b000 :
                    inst = std::make_unique<JalrInstruction>(pc, word);
                    break;
                default:
                    inst = std::make_unique<IllegalInstruction>(pc, word);
                    break;
            }
            break;
        case Opcode::BRANCH :
            
            // there is 6 types of branch isntructions 
            // which are separated by the funct3 field
            switch(funct3) {
                case 0b000 : // beq
                    inst = std::make_unique<BranchInstruction<std::equal_to<>, uint32_t>>(pc, word);
                    break;
                case 0b001 : // bne
                    inst = std::make_unique<BranchInstruction<std::not_equal_to<>, uint32_t>>(pc, word);
                    break;
                case 0b100 : // blt
                    inst = std::make_unique<BranchInstruction<std::less<>, int32_t>>(pc, word);
                    break;
                case 0b110 : // bltu
                    inst = std::make_unique<BranchInstruction<std::less<>, uint32_t>>(pc, word);
                    break;
                case 0b101 : // bge
                    inst = std::make_unique<BranchInstruction<std::greater_equal<>, int32_t>>(pc, word);
                    break;
                case 0b111: // bgeu
                    inst = std::make_unique<BranchInstruction<std::greater_equal<>, uint32_t>>(pc, word);
                    break;
                default:
                    inst = std::make_unique<IllegalInstruction>(pc, word);
                    break;
            }
            break;
        case Opcode::LOAD :
            // there is 5 types of load instructions
            // which are separated by the funct3 field
            switch(funct3) {
                case 0b000 : // lb
                    inst = std::make_unique<LoadInstruction<int8_t>>(pc, word);
                    break; 
                case 0b100 : // lbu
                    inst = std::make_unique<LoadInstruction<uint8_t>>(pc, word);
                    break;
                case 0b001 : // lh
                    inst = std::make_unique<LoadInstruction<int16_t>>(pc, word);
                    break; 
                case 0b101 : // lhu
                    inst = std::make_unique<LoadInstruction<uint16_t>>(pc, word);
                    break;
                case 0b010 : // lw
                    inst = std::make_unique<LoadInstruction<int32_t>>(pc, word);
                    break;
                default :
                    inst = std::make_unique<IllegalInstruction>(pc, word);
                    break;
            }
            break;
        case Opcode::STORE :
            // there is 3 types of load instructions
            // which are separated by the funct3 field
            switch(funct3) {
                case 0b000 : // sb
                    inst = std::make_unique<StoreInstruction<uint8_t>>(pc, word);
                    break;
                case 0b001 :
                    inst = std::make_unique<StoreInstruction<uint16_t>>(pc, word);
                    break;
                case 0b010 :
                    inst = std::make_unique<StoreInstruction<uint32_t>>(pc, word);
                    break;
                default:
                    inst = std::make_unique<IllegalInstruction>(pc, word);
                    break;
            }
            break;
        case Opcode::OP_IMM : {
            
            // the I-type instructions 
            switch(funct3) {
                case 0b000 : // addi
                    inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::ADD);
                    break; 
                case 0b111 : // andi
                    inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::AND);
                    break;
                case 0b110 : // ori
                    inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::OR);
                    break; 
                case 0b100 : // xori
                    inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::XOR);
                    break;
                case 0b010 : // slti
                    inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::SLT);
                    break;
                case 0b011 : // sltiu
                    inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::SLTU);
                    break;
                case 0b001 : // slli
                    switch(funct7) {
                        case 0b0000000 :
                            inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::SLL);
                            break;
                        default :
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                case 0b101 :
                    switch(funct7) {
                        case 0b0000000 : //srl
                            inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::SRL);
                            break;
                        case 0b0100000 : //sra
                            inst = std::make_unique<ImmediateOpInstruction>(pc, word, AluOp::SRA);
                            break;
                        default :
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                default:
                    inst = std::make_unique<IllegalInstruction>(pc, word);
                    break;
            }
            break;
        }
        case Opcode::OP : {
                        // the I-type instructions 
            switch(funct3) {

                case 0b000 : 
                    switch(funct7) {
                        case 0b0000000 : // add
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::ADD);
                            break;
                        case 0b0100000 : // sub
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::SUB);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                    }
                    break;
                case 0b111 : // and
                    switch(funct7) {
                        case 0b0000000 :
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::AND);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                case 0b110 : // or
                    switch(funct7) {
                        case 0b0000000 : 
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::OR);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                case 0b100 : // xor
                    switch(funct7) {
                        case 0b0000000 : 
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::XOR);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                case 0b001 : // sll
                    switch(funct7) {
                        case 0b0000000 : 
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::SLL);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                case 0b101 : 
                    switch(funct7) {
                        case 0b0000000 : // srl
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::SRL);
                            break;
                        case 0b0100000 : // sra
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::SRA);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                    }
                    break;
                case 0b010 : //slt
                    switch(funct7) {
                        case 0b0000000 : 
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::SLT);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                case 0b011 :
                    switch(funct7) {
                        case 0b0000000 : 
                            inst = std::make_unique<RegisterOpInstruction>(pc, word, AluOp::SLTU);
                            break;
                        default:
                            inst = std::make_unique<IllegalInstruction>(pc, word);
                            break;
                    }
                    break;
                default:
                    inst = std::make_unique<IllegalInstruction>(pc, word);
                    break;
            }
            break;
        }
        case Opcode::MISC_MEM : {
            switch(funct3) {
                case 0b000 :
                    inst = std::make_unique<FenceInstruction>(pc, word);
                    break;
                default:
                    inst = std::make_unique<IllegalInstruction>(pc, word);
                    break;
            }
            break;
        }
        case Opcode::SYSTEM : {
            if(rvsim::bits(word, 31, 0) == 0x00000073 || 
                        rvsim::bits(word, 31, 0) == 0x00100073 ) {

                inst = std::make_unique<SystemInstruction>(pc, word);
            }
            else {
                inst = std::make_unique<IllegalInstruction>(pc, word);
            }
            break;
        }
        default :
            inst = std::make_unique<IllegalInstruction>(pc, word);
            break;
    }
    

    return inst;

}


}
#endif