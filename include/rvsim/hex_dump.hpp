#ifndef HEX_DUMP_HPP
#define HEX_DUMP_HPP

#include <string>
#include <sstream>

#include "rvsim/bus.hpp"

namespace rvsim {


std::string hex_dump(Bus& bus, Addr start, std::size_t length);

}

#endif