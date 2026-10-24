#ifndef DECODE_HPP
#define DECODE_HPP

#include "rvsim/types.hpp"

namespace rvsim {


// function used to decode instructions 
// using the opcode and func3 and func7
std::unique_ptr<Instruction> decode(Word word, Addr pc);


}
#endif