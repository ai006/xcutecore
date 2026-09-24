#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>

#include "rvsim/types.hpp"
#include "rvsim/device.hpp"

namespace rvsim {

//RAM that holds data and instructions
class Memory : public Device {
    
    public:
        explicit Memory(std::size_t mem_alloc_size);
        ~Memory();
        std::size_t size() const override;
        Word read(Addr offset, std::size_t width) override;
        void write(Addr offset, std::size_t width, Word data) override;


    private:
        std::vector<std::uint8_t> RAM;

};
}

#endif