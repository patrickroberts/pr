#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <expected>
#include <optional>
#include <system_error>
#include <utility>

namespace pr {

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class file {
  std::optional<int> descriptor;

  constexpr explicit file(int fd) : descriptor(fd) {}

  [[nodiscard]] static constexpr auto file_or_error_code_from(int fd)
      -> std::expected<file, std::error_code> {
    if (fd == -1) {
      return std::unexpected(std::make_error_code(std::errc(errno)));
    }

    return file{fd};
  }

public:
  constexpr file(file &&other) noexcept
      : descriptor(std::move(other).release()) {}

  constexpr auto operator=(file &&other) noexcept -> file & {
    std::destroy_at(this);
    std::construct_at(this, std::move(other));
    return *this;
  }

  constexpr ~file() {
    if (not *this) {
      return;
    }

    close(**this);
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return descriptor.has_value();
  }

  [[nodiscard]] constexpr auto get() const noexcept -> std::optional<int> {
    return descriptor;
  }

  [[nodiscard]] constexpr auto operator*() const noexcept -> int {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *descriptor;
  }

  [[nodiscard]] constexpr auto release() && noexcept -> std::optional<int> {
    return std::exchange(descriptor, std::nullopt);
  }

  [[nodiscard]] static auto try_open(const char *path, int flags)
      -> std::expected<file, std::error_code> {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return file_or_error_code_from(open(path, flags));
  }

  [[nodiscard]] static auto try_open(const char *path, int flags, mode_t mode)
      -> std::expected<file, std::error_code> {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return file_or_error_code_from(open(path, flags, mode));
  }
};

} // namespace pr
