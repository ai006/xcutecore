#include <format>
#include <iostream>
#include <iomanip>

#include "rvsim/version.hpp"
#include "rvsim/types.hpp"
#include "rvsim/bits.hpp"
#include "rvsim/device.hpp"
#include "rvsim/memory.hpp"

using namespace rvsim;

int main() {
  std::cout << "rvsim " << rvsim::version() << '\n';
  
  return 0;
}
