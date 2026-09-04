#pragma once

#include <charconv>
#include <concepts>
#include <expected>
#include <string_view>
#include <system_error>

namespace pr {

template <std::integral T>
constexpr auto try_from_chars(std::string_view chars)
    -> std::expected<T, std::error_code> {
  T value;
  const auto result = std::from_chars(std::to_address(chars.begin()),
                                      std::to_address(chars.end()), value);

  if (result.ec != std::errc{}) {
    return std::unexpected(std::make_error_code(result.ec));
  }

  return value;
}

} // namespace pr
