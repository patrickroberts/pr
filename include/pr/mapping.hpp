#pragma once

#include <sys/mman.h>

#include <expected>
#include <ranges>

namespace pr {

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class mapping : public std::ranges::view_interface<mapping> {
  std::span<std::byte> memory;

  constexpr explicit mapping(std::byte *addr, std::size_t len) noexcept
      : memory(addr, len) {}

public:
  constexpr mapping(mapping &&other) noexcept
      : memory(std::move(other).release()) {}

  constexpr auto operator=(mapping &&other) noexcept -> mapping & {
    std::destroy_at(this);
    std::construct_at(this, std::move(other));
    return *this;
  }

  constexpr ~mapping() {
    if (not *this) {
      return;
    }

    munmap(data(), size());
  }

  [[nodiscard]] constexpr auto begin() const noexcept -> std::byte * {
    return std::to_address(memory.begin());
  }

  [[nodiscard]] constexpr auto end() const noexcept -> std::byte * {
    return std::to_address(memory.end());
  }

  [[nodiscard]] constexpr auto release() && noexcept -> std::span<std::byte> {
    return std::exchange(memory, {});
  }

  struct configuration {
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_ANONYMOUS | MAP_PRIVATE;
    int fd = -1;
    off_t offset = 0;
  };

  [[nodiscard]] static auto try_mmap(void *addr, std::size_t len) noexcept
      -> std::expected<mapping, std::error_code> {
    return try_mmap(addr, len, {});
  }

  [[nodiscard]] static auto try_mmap(void *addr, std::size_t len,
                                     configuration config) noexcept
      -> std::expected<mapping, std::error_code> {
    addr = mmap(addr, len, config.prot, config.flags, config.fd, config.offset);

    if (addr == MAP_FAILED) {
      return std::unexpected(std::make_error_code(std::errc(errno)));
    }

    return mapping{static_cast<std::byte *>(addr), len};
  }
};

} // namespace pr
