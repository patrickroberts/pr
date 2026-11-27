#include "last_error_posix.hpp"
#include "unexpected.hpp"

#include <pr/file.hpp>
#include <pr/result.hpp>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

namespace pr {
namespace {

inline auto try_get_file_access(file::access_mode access)
    -> std::expected<int, std::error_code> {
  using enum file::access_mode;

  switch (access) {
  case read:
    return O_RDONLY;
  case write:
    return O_WRONLY;
  case read_write:
    return O_RDWR;
  }

  return pr::unexpected(std::errc::invalid_argument);
}

inline auto try_get_file_handle(const char *path, int flags,
                                file::creation_mode creation)
    -> std::expected<int, std::error_code> {
  constexpr mode_t default_file_mode = 0666;
  using enum file::creation_mode;

  switch (creation) {
  case open_existing:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return open(path, flags);
  case open_or_create:
    // NOLINTNEXTLINE(bugprone-signed-bitwise,cppcoreguidelines-pro-type-vararg)
    return open(path, flags | O_CREAT, default_file_mode);
  case create_new:
    // NOLINTNEXTLINE(bugprone-signed-bitwise,cppcoreguidelines-pro-type-vararg)
    return open(path, flags | O_CREAT | O_EXCL, default_file_mode);
  }

  return pr::unexpected(std::errc::invalid_argument);
}

} // namespace
} // namespace pr

void pr::file::try_close() const noexcept {
  if (not valueless_after_move()) {
    close(handle);
  }
}

auto pr::file::try_open(const std::filesystem::path &path,
                        open_options options) noexcept
    -> std::expected<file, std::error_code> {
  const auto access = PR_MAYBE(pr::try_get_file_access(options.access));
  const auto handle =
      PR_MAYBE(pr::try_get_file_handle(path.c_str(), access, options.creation));

  if (handle == -1) {
    return std::unexpected{pr::last_error()};
  }

  return file{handle};
}

auto pr::try_set_length(const file &fd, std::int64_t length) noexcept
    -> std::expected<void, std::error_code> {
  if (not std::in_range<off_t>(length)) {
    return pr::unexpected(std::errc::file_too_large);
  }

  if (ftruncate(fd.native_handle(), static_cast<off_t>(length)) == -1) {
    return std::unexpected{pr::last_error()};
  }

  return {};
}

auto pr::try_seek_end(const file &fd) noexcept
    -> std::expected<std::int64_t, std::error_code> {
  const auto position = lseek(fd.native_handle(), 0, SEEK_END);

  if (position == -1) {
    return std::unexpected{pr::last_error()};
  }

  if (not std::in_range<std::int64_t>(position)) {
    return pr::unexpected(std::errc::value_too_large);
  }

  return static_cast<std::int64_t>(position);
}
