/**
 * Entry point.
 */

#include "os_agnostic/MarqueeConsole.hpp"
#include <iostream>

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  MarqueeConsole console;
  console.run();
  std::cout << "Finished Execution!\n";
  return 0;
}
