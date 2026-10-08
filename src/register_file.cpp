#include <cassert>

#include "rvsim/register_file.hpp"

namespace rvsim {

std::string_view abi_name(RegIndex index) {

    assert(index < RegisterFile::numRegisters);
    static constexpr std::array<std::string_view, RegisterFile::numRegisters> regNames{
        "zero", "ra", "sp", "gp",
        "tp",   "t0", "t1", "t2",
        "s0",   "s1", "a0", "a1",
        "a2",   "a3", "a4", "a5",
        "a6",   "a7", "s2", "s3",
        "s4",   "s5", "s6", "s7",
        "s8",   "s9", "s10","s11",
        "t3",   "t4",   "t5", "t6"
    };
    return regNames[index];
}


void
RegisterFile::write(RegIndex index, Word value) {

    //make not trying to writes are not to zero
    if(index == 0)
        return;

    //writing to registers 32 or above is not allowed
    assert(index < numRegisters);
    //write to reg
    registers[index] = value;

}

Word
RegisterFile::read(RegIndex index) const {
    assert(index < numRegisters);
    //read from register
    return registers[index];
}


}