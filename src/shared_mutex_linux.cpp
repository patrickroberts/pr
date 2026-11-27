#include "last_error_posix.hpp"

#include <pr/shared_mutex.hpp>

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <ctime>
#include <limits>

void pr::wait(std::uint32_t *address, std::uint32_t expected) {
  constexpr std::timespec *timeout = nullptr;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (syscall(SYS_futex, address, FUTEX_WAIT, expected, timeout) == 0)
      [[likely]] {
    return;
  }

  const auto error = pr::last_error();

  if (error.value() == EAGAIN or error.value() == EINTR) [[likely]] {
    return;
  }

  throw std::system_error{error, "FUTEX_WAIT"};
}

void pr::wake_all(std::uint32_t *address) {
  constexpr std::uint32_t waiters = std::numeric_limits<int>::max();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (syscall(SYS_futex, address, FUTEX_WAKE, waiters) >= 0) [[likely]] {
    return;
  }

  const auto error = pr::last_error();
  throw std::system_error{error, "FUTEX_WAKE"};
}
