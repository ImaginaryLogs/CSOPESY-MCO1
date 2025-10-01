/**
 * Entry point.
 */

#include "os_agnostic/MarqueeConsole.hpp"
#include <iostream>

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  // Welcome banner
  std::cout << "\n\n***********************************************\n\n"
            << "Welcome to the Marquee Console!\n\n"
            << "Developers:\n"
            << " - Christian Joseph C. Bunyi\n"
            << " - Roan Cedric V. Campo\n"
            << " - Enzo Rafael S. Chan\n"
            << " - Mariella Jeanne A. Dellosa\n\n"
            << "Tip: Type 'help' to see available commands.\n"
            << "\n***********************************************\n"
            << std::flush;

  MarqueeConsole console;
  console.run();

  std::cout << "Finished Execution!\n";
  return 0;
}
