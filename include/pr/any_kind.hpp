#pragma once

namespace pr {

enum class any_kind : unsigned char {
  // clang-format off
  input         = 0b00000000,
  forward       = 0b00000001,
  bidirectional = 0b00000011,
  random_access = 0b00000111,
  contiguous    = 0b00001111,
  borrowed      = 0b00010000,
  constant      = 0b00100000,
  copyable      = 0b01000000,
  sized         = 0b10000000,
  // clang-format on
};

constexpr auto operator|(any_kind l, any_kind r) noexcept -> any_kind {
  return any_kind(static_cast<unsigned>(l) | static_cast<unsigned>(r));
}

constexpr auto operator&(any_kind l, any_kind r) noexcept -> any_kind {
  return any_kind(static_cast<unsigned>(l) & static_cast<unsigned>(r));
}

constexpr auto operator^(any_kind l, any_kind r) noexcept -> any_kind {
  return any_kind(static_cast<unsigned>(l) ^ static_cast<unsigned>(r));
}

constexpr auto operator~(any_kind k) noexcept -> any_kind {
  return any_kind(~static_cast<unsigned>(k));
}

constexpr auto operator|=(any_kind &l, any_kind r) noexcept -> any_kind & {
  return l = l | r;
}

constexpr auto operator&=(any_kind &l, any_kind r) noexcept -> any_kind & {
  return l = l & r;
}

constexpr auto operator^=(any_kind &l, any_kind r) noexcept -> any_kind & {
  return l = l ^ r;
}

} // namespace pr
