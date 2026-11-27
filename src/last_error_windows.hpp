#pragma once

#include <Windows.h>

#include <system_error>

namespace pr {
namespace {

inline auto last_error() noexcept -> std::error_code {
  return {static_cast<int>(GetLastError()), std::system_category()};
}

} // namespace
} // namespace pr
