#ifndef BUS_HPP
#define BUS_HPP

#include <cassert>
#include <format>
#include <vector>
#include <memory>
#include <stdexcept>

#include "rvsim/device.hpp"
#include "rvsim/memory_fault.hpp"
#include "rvsim/types.hpp"

namespace rvsim{

// defining the width the bus can work with
template <typename T>
concept BusValue = std::same_as<T, std::uint8_t> ||
                   std::same_as<T, std::uint16_t> ||
                   std::same_as<T, std::uint32_t>;


class Bus {

    public:
        //assign device to memory region and check if not used
        void map(Addr base, std::unique_ptr<Device> _device);
        //read instructions
        Word fetch(Addr address);
        //read data from device
        Word read(Addr address, std::size_t width);
        //write data to device
        void write(Addr address, std::size_t width, Word data);

    private:
        //struct for one mapping (a base addr and device)
        struct MappedDevices{
            Addr baseAddress;
            std::unique_ptr<Device> device;
        };

        //struct for found mapping
        struct FoundDevice{
            Addr deviceOffset;
            Device& device;
        };

        //list of mappings
        std::vector<MappedDevices> listOfMappedDevices;

        //used to search for the mapped device
        Bus::FoundDevice lookup(Addr address, std::size_t width, AccessType access);
        //used to make sure new device wont overlap
        bool overlaps(Addr base, std::size_t size) const;

    public:
        // Creating templates for different widths
        template <BusValue T>
        T read(Addr address) {
            return static_cast<T>(read(address, sizeof(T)));
        }
    
        template <BusValue T>
        void write(Addr address, T data) {
            write(address, sizeof(T), data);
        }
};
}

#endif