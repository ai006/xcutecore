#ifndef MEMORY_FAULT_HPP
#define MEMORY_FAULT_HPP

#include <stdexcept>

#include "rvsim/types.hpp"

namespace rvsim{
    

enum class AccessType {
    Fetch,
    Read,
    Write
};

std::string_view to_string(AccessType type);

// Thrown when an access touches bytes that do not exist.
// Memory throws it with the offset it was given
class MemoryFault : public std::runtime_error {

    public:
        MemoryFault(Addr _offset, std::size_t _width, AccessType _access);
        
        Addr 
        getAddr() const {
            return offset;
        }

        std::size_t
        getWidth() const {
            return width;
        }

        AccessType
        getAccessType() const {
            return access;
        }

    private:
        static std::string report_fault(Addr addr, std::size_t width, AccessType type);

        Addr offset;
        std::size_t width;
        AccessType access;
};
}

#endif