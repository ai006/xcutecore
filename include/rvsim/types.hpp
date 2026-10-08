#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

namespace rvsim{
    
//32-bit unsigned integer
using Word = uint32_t;

//signed 32-bit integer
using Sword = int32_t;

//unsigned 32-bit memory address
using Addr = uint32_t;

//a register number 0 to 31
using RegIndex = uint8_t;

}

#endif
