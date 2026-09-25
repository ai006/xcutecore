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
    check(offset, width, AccessType::Read);

    uint32_t byteSize = 8;
    uint32_t numBits = (uint32_t) (width * byteSize);
    Word data = 0;
    for(uint32_t lsb = 0; lsb < numBits; lsb += byteSize, offset++){

        Word byteRead = RAM[offset];
        data = data | (byteRead << lsb);
    }
    return data;
}

void
Memory::write(Addr offset, std::size_t width, Word data) {

    check(offset, width, AccessType::Write);

    uint32_t byteSize = 8;
    uint32_t numBits = (uint32_t) (width * byteSize);
    for(uint32_t lsb = 0; lsb < numBits; lsb += byteSize, offset++){
        uint32_t msb = lsb + byteSize - 1;
        Word tmp = bits(data, msb, lsb);
        uint8_t byte = static_cast<uint8_t>(tmp);
        RAM[offset] = byte;
    }
}

void
Memory::check(Addr offset, std::size_t width, AccessType type) const {
  // A bad width can only come from simulator code: assert.
  assert(width == 1 || width == 2 || width == 4);
 
  // A bad address can come from the guest program: throw.
  // Checks the last byte
  if (offset + width > size()) {
    throw MemoryFault(offset, width, type);
  }
}
}
