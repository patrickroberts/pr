#include "last_error_windows.hpp"

#include <pr/shared_mutex.hpp>

#include <Windows.h>

void pr::wait(std::uint32_t *address, std::uint32_t expected) {
  if (WaitOnAddress(address, std::addressof(expected), sizeof(expected),
                    INFINITE) != FALSE) [[likely]] {
    return;
  }

  const auto error = pr::last_error();
  throw std::system_error{error, "WaitOnAddress"};
}

void pr::wake_all(std::uint32_t *address) { WakeByAddressAll(address); }
