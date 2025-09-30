/**
 * Windows implementation of Scanner
 */
#include "../os_dependent/Scanner.hpp"

#if defined(_WIN32)
#include <conio.h>

struct Scanner::Impl {
  Impl() {}
  ~Impl() {}
  int poll() {
    if (_kbhit()) {
      int ch = _getch();
      if (ch == '\r') ch = '\n'; // map CR to NL
      return ch;
    }
    return -1;
  }
};

Scanner::Scanner() : impl(new Impl()) {}
Scanner::~Scanner() { delete impl; }
int Scanner::poll() { return impl->poll(); }

#else
// Non-windows translation unit should be empty to avoid duplicate symbols.
struct DummyWinScanner {};
#endif
