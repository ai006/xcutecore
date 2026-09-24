#include <cassert>

#include "rvsim/bits.hpp"
#include "rvsim/memory.hpp"

namespace rvsim {

Memory::Memory(std::size_t mem_alloc_size) {

    RAM.assign(mem_alloc_size, 0);
}

Memory::~Memory() {
    RAM.clear();
}

std::size_t
Memory::size() const {
    return RAM.size();
}

Word
Memory::read(Addr offset, std::size_t width) {
    //make sure offset is less than
    //allocated memory
    assert(offset < size());
    assert(width >= 1);
    //get byte
    uint8_t byte = RAM[offset];
    //sign extend byte and return it
    return Word{byte};
}

void
Memory::write(Addr offset, std::size_t width, Word data) {

    assert(offset < size());
    assert(width >= 1);
    uint8_t byte = static_cast<uint8_t>(data);
    RAM[offset] = byte;
}
}