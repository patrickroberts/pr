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

    close(native_handle());
  }

  [[nodiscard]] constexpr auto valueless_after_move() const noexcept -> bool {
    return descriptor == -1;
  }

  [[nodiscard]] constexpr auto native_handle() const noexcept -> int {
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

[[nodiscard]] auto try_ftruncate(const file &fd, off_t length) noexcept
    -> std::expected<void, std::error_code> {
  const auto result = ::ftruncate(fd.native_handle(), length);

  if (result == -1) {
    return std::unexpected(std::make_error_code(std::errc(errno)));
  }

  return {};
}

[[nodiscard]] auto try_lseek(const file &fd, off_t offset, int whence) noexcept
    -> std::expected<off_t, std::error_code> {
  const auto result = ::lseek(fd.native_handle(), offset, whence);

  if (result == -1) {
    return std::unexpected(std::make_error_code(std::errc(errno)));
  }

  return result;
}

} // namespace pr
