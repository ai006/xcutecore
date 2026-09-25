#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>

#include "rvsim/types.hpp"
#include "rvsim/device.hpp"
#include "rvsim/memory_fault.hpp"

namespace rvsim {

//Memory that will hold data and instructions
class Memory : public Device {
    
    public:
        explicit Memory(std::size_t mem_alloc_size);
        ~Memory();
        //size of the memory allocated
        std::size_t size() const override;
        Word read(Addr offset, std::size_t width) override;
        void write(Addr offset, std::size_t width, Word data) override;


    private:
        // Checks the width, and throws MemoryFault unless the whole access fits.
        void check(Addr offset, std::size_t width, AccessType type) const;

        std::vector<std::uint8_t> RAM;

};
}

#endif