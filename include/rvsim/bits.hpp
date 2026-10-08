#ifndef BITS_HPP
#define BITS_HPP

#include <cassert>
#include <iomanip>
#include <string>

#include "types.hpp"

namespace rvsim {

//Takes a Word and two bit positions 
//with 0 ≤ lo ≤ hi ≤ 31. Returns the bits
//from hi down to lo as a Word, 
[[nodiscard]]
constexpr Word bits(Word value, uint32_t hi, uint32_t lo) noexcept {
    assert(hi >= lo);
    assert(hi <= 31);

    //remove all the lower bits
    value = value >> lo;

    //remove the higher bits with masking
    for(uint32_t i = 31; i > (hi-lo); i--){
        value = value & ~(1u << i);
    }
    return value;
}

//check if bit is 1 or 0
[[nodiscard]]
constexpr bool bit(Word value, uint32_t pos) noexcept {
    assert(pos <= 31);
    return value & (1u << pos);
}

//singed extend the value
[[nodiscard]]
constexpr Word sign_extend(Word value, uint32_t width) noexcept {

    assert(width >= 1);
    assert(width <= 32);

    // sign bit is at width-1
    const bool negative = bit(value, width - 1);   

    for (uint32_t i = width; i < 32; i++) {
        if (negative)
            value |= (Word{1} << i);    // set upper bits
        else
            value &= ~(Word{1} << i);   // clear upper bits
    }
    return value;
}

//print the binary of the word
inline std::string to_binary_string(Word value) {
    std::string binary = "";
    for(uint32_t i = 32; i > 0; i--){
        if(bit(value, (i - 1u)))
            binary += '1';
        else
            binary += '0';

        if((i-1u) % 4 == 0 && i != 1)
            binary += ' ';
    }
    return binary;
}
}

#endif