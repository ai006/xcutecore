#include <iostream>
#include <iomanip>

#include "rvsim/version.hpp"
#include "rvsim/types.hpp"
#include "rvsim/bits.hpp"

int main() {
  std::cout << "rvsim " << rvsim::version() << '\n';
  return 0;
}
