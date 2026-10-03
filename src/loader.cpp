#include "rvsim/loader.hpp"

namespace rvsim {


void 
load_bytes(Bus& bus, Addr start, std::span<const std::uint8_t> bytes) {

    for(std::size_t i = 0; i < bytes.size(); i++){
        Addr address = static_cast<Addr>(start + i);
        bus.write(address, bytes[i]);
    }

}
}