#ifndef ARCHSTATE_HPP
#define ARCHSTATE_HPP

#include <iostream>

#include "rvsim/register_file.hpp"
#include "rvsim/types.hpp"


namespace rvsim {

// struct which holds the architectural state
struct ArchState{
    // program counter register
    Addr pc = 0;
    //register file for xCuteCore 
    RegisterFile regs;

    bool operator == (const ArchState& archState) const = default;
};

//overloading this function to output the pc and register file
std::ostream& operator<<(std::ostream& os, const ArchState& archState);

}

#endif