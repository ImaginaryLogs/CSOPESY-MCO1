/**
 * Entry point.
 */

#include "os_agnostic/MarqueeConsole.hpp"
#include <iostream>

int main() {
  MarqueeConsole console;
  console.run();
  std::cout << "Finished Execution!\n";
  return 0;
}
