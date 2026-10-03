#ifndef LOADER_HPP
#define LOADER_HPP

#include <cstdint>
#include <span>

#include "rvsim/bus.hpp"

namespace rvsim {

// function used to Put a block of bytes onto the bus
// one byte at a time, starting at an address
void load_bytes(Bus& bus, Addr start, std::span<const std::uint8_t> bytes);
    
}
#endif