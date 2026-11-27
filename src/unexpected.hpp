#pragma once

#include <expected>
#include <system_error>

namespace pr {
namespace {

inline auto unexpected(std::errc error_condition) noexcept
    -> std::unexpected<std::error_code> {
  return std::unexpected{std::make_error_code(error_condition)};
}

} // namespace
} // namespace pr
