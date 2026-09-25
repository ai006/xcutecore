#include <format>
#include <string>

#include "rvsim/memory_fault.hpp"

namespace rvsim {

std::string_view to_string(AccessType type) {
  switch (type) {
    case AccessType::Fetch:
      return "fetch";
    case AccessType::Read:
      return "read";
    case AccessType::Write:
      return "write";
  }
  return "unknown";  // unreachable, but GCC wants a return after the switch
}


MemoryFault::MemoryFault(Addr _offset, std::size_t _width, AccessType _access) :
                        std::runtime_error(report_fault(_offset, _width, _access)),
                            offset(_offset),
                                width(_width),
                                    access(_access) {


}

std::string
MemoryFault::report_fault(Addr addr, std::size_t width, AccessType type) {
    return std::format(
                        "memory fault: {}-byte {} at 0x{:08x}", 
                        width, 
                        to_string(type),
                        addr
                    );
}
}