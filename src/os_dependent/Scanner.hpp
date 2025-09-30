/**
 * OS-dependent keyboard scanner (single key poll).
 * Windows: _kbhit/_getch
 * POSIX: termios raw + select + read
 */
#pragma once

class Scanner {
public:
  Scanner();
  ~Scanner();
  // returns -1 if no key; otherwise returns an unsigned char value (0..255) promoted to int
  int poll();
private:
  struct Impl;
  Impl* impl;
};
