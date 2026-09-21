#include <iostream>

#include "rvsim/version.hpp"

int main() {
  std::cout << "rvsim " << rvsim::version() << '\n';
  return 0;
}
