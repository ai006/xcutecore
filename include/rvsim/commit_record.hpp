#ifndef COMMIT_RECORD_HPP
#define COMMIT_RECORD_HPP

#include <optional>

#include "types.hpp"

namespace rvsim {

// struct to hold which register changed and its value
// "A register changed": which register (rd) and its new value.
struct RegWrite {
    RegIndex rd;
    Word value;
    bool operator == (const RegWrite& reg_Write) const = default;
};

// struct to hold an account of the memory change
// "Memory changed": the addr, the width in bytes 
// (1, 2 or 4), and the value written.
struct MemWrite {
    Addr addr;
    std::size_t width;
    Word value;
    bool operator == (const MemWrite& mem_Write) const = default;
};

// The Main information holder: pc, word, a maybe-register-change 
// and a maybe-memory-change.
struct CommitRecord {
    Addr pc = 0;
    Word word = 0;
    std::optional<RegWrite> regWrite;
    std::optional<MemWrite> memWrite;
    bool operator == (const CommitRecord& commit_Record) const = default;
};

}


#endif