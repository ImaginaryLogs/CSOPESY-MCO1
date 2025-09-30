/**
 * POSIX implementation of Scanner
 */
#include "../os_dependent/Scanner.hpp"

#if !defined(_WIN32)
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

struct Scanner::Impl {
  termios old{};
  bool ok{false};
  Impl() {
    if (tcgetattr(STDIN_FILENO, &old) == 0) {
      termios raw = old;
      raw.c_lflag &= ~(ICANON | ECHO);
      raw.c_cc[VMIN]  = 0;
      raw.c_cc[VTIME] = 0;
      tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
      ok = true;
    }
  }
  ~Impl() {
    if (ok) tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
  }
  int poll() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{};
    tv.tv_sec = 0; tv.tv_usec = 10000; // 10ms
    int r = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    if (r > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
      unsigned char c;
      ssize_t n = ::read(STDIN_FILENO, &c, 1);
      if (n == 1) return static_cast<int>(c);
    }
    return -1;
  }
};

Scanner::Scanner() : impl(new Impl()) {}
Scanner::~Scanner() { delete impl; }
int Scanner::poll() { return impl->poll(); }

#else
// Windows builds should use the other translation unit
struct DummyPosixScanner {};
#endif
