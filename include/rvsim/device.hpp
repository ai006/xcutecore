#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "rvsim/types.hpp"

namespace rvsim {

class Device{

    public:
        virtual ~Device() {
            
        }
        virtual std::size_t size() const = 0;
        virtual Word read(Addr offset, std::size_t width) = 0;
        virtual void write(Addr offset, std::size_t width, Word data) = 0;

};
}

#endif