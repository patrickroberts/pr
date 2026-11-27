#pragma once

#ifndef PR_FILE_NATIVE_HANDLE_TYPE
#error "PR_FILE_NATIVE_HANDLE_TYPE must be defined by the build system"
#endif
#ifndef PR_FILE_INVALID_NATIVE_HANDLE
#error "PR_FILE_INVALID_NATIVE_HANDLE must be defined by the build system"
#endif

#include <cstdint>
#include <expected>
#include <filesystem>
#include <system_error>
#include <utility>

namespace pr {

class file {
  using native_handle_type = PR_FILE_NATIVE_HANDLE_TYPE;
  static constexpr native_handle_type invalid_native_handle =
      PR_FILE_INVALID_NATIVE_HANDLE;

  native_handle_type handle;

  explicit file(native_handle_type handle) noexcept : handle(handle) {}

  void try_close() const noexcept;

public:
  enum class access_mode : std::uint8_t {
    read,
    write,
    read_write,
  };

  enum class creation_mode : std::uint8_t {
    open_existing,
    open_or_create,
    create_new,
  };

  struct open_options {
    access_mode access{access_mode::read};
    creation_mode creation{creation_mode::open_existing};
  };

  file(const file &) = delete;
  file(file &&other) noexcept
      : handle(std::exchange(other.handle, invalid_native_handle)) {}

  auto operator=(const file &) -> file & = delete;
  auto operator=(file &&other) noexcept -> file & {
    if (this == std::addressof(other)) {
      return *this;
    }

    try_close();
    handle = std::exchange(other.handle, invalid_native_handle);
    return *this;
  }

  ~file() { try_close(); }

  [[nodiscard]] auto valueless_after_move() const noexcept -> bool {
    return handle == invalid_native_handle;
  }

  [[nodiscard]] auto native_handle() const noexcept -> native_handle_type {
    return handle;
  }

  [[nodiscard]] static auto try_open(const std::filesystem::path &path) noexcept
      -> std::expected<file, std::error_code> {
    return try_open(path, {});
  }

  [[nodiscard]] static auto try_open(const std::filesystem::path &path,
                                     open_options options) noexcept
      -> std::expected<file, std::error_code>;
};

[[nodiscard]] auto try_set_length(const file &fd, std::int64_t length) noexcept
    -> std::expected<void, std::error_code>;

[[nodiscard]] auto try_seek_end(const file &fd) noexcept
    -> std::expected<std::int64_t, std::error_code>;

} // namespace pr
