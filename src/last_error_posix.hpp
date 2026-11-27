#pragma once

#include <cerrno>
#include <system_error>

namespace pr {
namespace {

inline auto last_error() noexcept -> std::error_code {
  return {errno, std::generic_category()};
}

} // namespace
} // namespace pr
