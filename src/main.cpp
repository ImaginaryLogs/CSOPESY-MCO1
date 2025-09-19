
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "os_agnostic/Marquee.cpp"

int main()
{
  MarqueeConsole console;

  console.run();
  
  std::cout << "Finished Execution!";

  return 0;
}