
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "marquee.cpp"

int main()
{
  MarqueeConsole console;
  console.runLoop();
  return 0;
}