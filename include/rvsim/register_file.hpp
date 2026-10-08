#ifndef REGISTER_FILE_HPP
#define REGISTER_FILE_HPP

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "rvsim/types.hpp"

namespace rvsim {

std::string_view abi_name(RegIndex index);

class RegisterFile {

    public:
        static constexpr std::size_t numRegisters = 32;

        // function to write to register
        void write(RegIndex index, Word value);
        // function to read from register
        Word read(RegIndex index) const;
        
        // function to compare two register files
        bool operator == (const RegisterFile& regfile) const {
            for(RegIndex index = 0; index < numRegisters; index++){
                if(regfile.registers[index] != registers[index]){
                    return false;
                }
            }
            return true;
        }

    private:
        
        
        std::array<Word, numRegisters> registers{};
        
};

}

#endif