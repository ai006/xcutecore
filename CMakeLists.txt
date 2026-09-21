cmake_minimum_required(VERSION 3.22)
project(rvsim LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RVSIM_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Warnings and sanitizers, applied only to targets that link this.
add_library(rvsim_options INTERFACE)
target_compile_options(rvsim_options INTERFACE
  -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
  -Wshadow -Wswitch-enum -Werror)
if(RVSIM_SANITIZE)
  target_compile_options(rvsim_options INTERFACE
    -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  target_link_options(rvsim_options INTERFACE -fsanitize=address,undefined)
endif()

add_subdirectory(src)
add_subdirectory(sim)

enable_testing()
add_subdirectory(tests)
