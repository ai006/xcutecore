#ifndef ALU_HPP
#define ALU_HPP

#include <stdexcept>
#include <string>

namespace rvsim {

// list of all the ALU operations
enum class AluOp {
    ADD,
    SUB,
    SLL,
    SLT,
    SLTU,
    XOR,
    SRL,
    SRA,
    OR,
    AND
};

// get the instructions mnemonic
[[nodiscard]] constexpr std::string 
extract_mnemonic_imm(AluOp aluOp) {
    
    std::string res = "";
    switch(aluOp) {
        case AluOp::ADD :
            res = "addi";
            break;
        case AluOp::SLL :
            res = "slli";
            break;
        case AluOp::SLT :
            res = "slti";
            break;
        case AluOp::SLTU :
            res = "sltiu";
            break;
        case AluOp::XOR :
            res = "xori";
            break;
        case AluOp::SRL :
            res = "srli";
            break;
        case AluOp::SRA :
            res = "srai";
            break;
        case AluOp::OR :
            res = "ori";
            break;
        case AluOp::AND :
            res = "andi";
            break;
        case AluOp::SUB :
        default:   
            throw std::invalid_argument("invalid ALU operation cannot extract mnemonic");
    }
    return res;
}

// get the instructions mnemonic
[[nodiscard]] constexpr std::string 
extract_mnemonic(AluOp aluOp) {
    
    std::string res = "";
    switch(aluOp) {
        case AluOp::ADD :
            res = "add";
            break;
        case AluOp::SUB :
            res = "sub";
            break;
        case AluOp::SLL :
            res = "sll";
            break;
        case AluOp::SLT :
            res = "slt";
            break;
        case AluOp::SLTU :
            res = "sltu";
            break;
        case AluOp::XOR :
            res = "xor";
            break;
        case AluOp::SRL :
            res = "srl";
            break;
        case AluOp::SRA :
            res = "sra";
            break;
        case AluOp::OR :
            res = "or";
            break;
        case AluOp::AND :
            res = "and";
            break;
        default:   
            throw std::invalid_argument("invalid ALU operation cannot extract mnemonic");
    }
    return res;
}






}


#endif