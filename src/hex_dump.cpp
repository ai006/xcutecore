#include <format>
#include <iostream>

#include "rvsim/bits.hpp"
#include "rvsim/hex_dump.hpp"

namespace rvsim {


//duymp memory 16 bytes per line
std::string 
hex_dump(Bus& bus, Addr start, std::size_t length) { 

    // empty range returns an empty string
    if(length == 0)
        return "";

    std::ostringstream buffer;
    buffer << std::hex << std::setfill('0');
    for(std::size_t i = 0; i < length; i++){

        Addr address = static_cast<Addr>(start+i);
        //this shows 16 bytes per line
        if(i % 16 == 0){
            if (buffer.str().empty())
                buffer << std::setw(8) << address <<":";
            else
                buffer <<'\n' <<std::setw(8) << address  <<":";
        }
        std::uint8_t byte = bus.read<std::uint8_t>(address);

        buffer << ' ' << std::setw(2) << static_cast<unsigned>(byte);

    }
    buffer << '\n';
    return buffer.str();
}
}