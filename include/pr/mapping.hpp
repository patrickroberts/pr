#pragma once

#include <pr/file.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <ranges>
#include <span>
#include <system_error>
#include <utility>

namespace pr {

class mapping : public std::ranges::view_interface<mapping> {
  std::span<std::byte> memory;

  explicit mapping(std::byte *addr, std::size_t len) noexcept
      : memory(addr, len) {}

  void try_unmap() const noexcept;

public:
  enum class access_mode : std::uint8_t {
    read,
    write,
    read_write,
  };

  enum class sharing_mode : std::uint8_t {
    shared,
    private_copy,
  };

  struct configuration {
    access_mode access{access_mode::read_write};
    sharing_mode sharing{sharing_mode::private_copy};
    const file *handle{};
    std::uint64_t offset{};
  };

  mapping(const mapping &) = delete;
  mapping(mapping &&other) noexcept : memory(std::exchange(other.memory, {})) {}

  auto operator=(const mapping &) -> mapping & = delete;
  auto operator=(mapping &&other) noexcept -> mapping & {
    if (this == std::addressof(other)) {
      return *this;
    }

    try_unmap();
    memory = std::exchange(other.memory, {});
    return *this;
  }

  ~mapping() { try_unmap(); }

  [[nodiscard]] auto begin() const noexcept -> std::byte * {
    return std::to_address(memory.begin());
  }

  [[nodiscard]] auto end() const noexcept -> std::byte * {
    return std::to_address(memory.end());
  }

  [[nodiscard]] static auto try_mmap(void *addr, std::size_t len) noexcept
      -> std::expected<mapping, std::error_code> {
    return try_mmap(addr, len, {});
  }

  [[nodiscard]] static auto try_mmap(void *addr, std::size_t len,
                                     configuration config) noexcept
      -> std::expected<mapping, std::error_code>;
};

} // namespace pr
