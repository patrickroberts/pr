#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <expected>
#include <system_error>
#include <utility>

namespace pr {

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class file {
  int descriptor;

  constexpr explicit file(int fd) : descriptor(fd) {}

  [[nodiscard]] static constexpr auto file_or_error_code_from(int fd) noexcept
      -> std::expected<file, std::error_code> {
    if (fd == -1) {
      return std::unexpected(std::make_error_code(std::errc(errno)));
    }

    return file{fd};
  }

public:
  constexpr file(file &&other) noexcept
      : descriptor(std::exchange(other.descriptor, -1)) {}

  constexpr auto operator=(file &&other) noexcept -> file & {
    std::destroy_at(this);
    std::construct_at(this, std::move(other));
    return *this;
  }

  constexpr ~file() {
    if (valueless_after_move()) {
      return;
    }

    close(**this);
  }

  [[nodiscard]] constexpr auto valueless_after_move() const noexcept -> bool {
    return descriptor == -1;
  }

  [[nodiscard]] constexpr auto operator*() const noexcept -> int {
    return descriptor;
  }

  [[nodiscard]] static auto try_open(const char *path, int flags) noexcept
      -> std::expected<file, std::error_code> {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return file_or_error_code_from(open(path, flags));
  }

  [[nodiscard]] static auto try_open(const char *path, int flags,
                                     mode_t mode) noexcept
      -> std::expected<file, std::error_code> {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return file_or_error_code_from(open(path, flags, mode));
  }
};

} // namespace pr
