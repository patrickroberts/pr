#include "last_error_windows.hpp"
#include "unexpected.hpp"

#include <pr/file.hpp>
#include <pr/result.hpp>

#include <Windows.h>

#include <utility>

namespace pr {
namespace {

inline auto try_get_file_access(file::access_mode access)
    -> std::expected<DWORD, std::error_code> {
  using enum file::access_mode;

  switch (access) {
  case read:
    return GENERIC_READ;
  case write:
    return GENERIC_WRITE;
  case read_write:
    return GENERIC_READ | GENERIC_WRITE;
  }

  return pr::unexpected(std::errc::invalid_argument);
}

inline auto try_get_file_creation(file::creation_mode creation)
    -> std::expected<DWORD, std::error_code> {
  using enum file::creation_mode;

  switch (creation) {
  case open_existing:
    return OPEN_EXISTING;
  case open_or_create:
    return OPEN_ALWAYS;
  case create_new:
    return CREATE_NEW;
  }

  return pr::unexpected(std::errc::invalid_argument);
}

} // namespace
} // namespace pr

void pr::file::try_close() const noexcept {
  if (not valueless_after_move()) {
    CloseHandle(handle);
  }
}

auto pr::file::try_open(const std::filesystem::path &path,
                        open_options options) noexcept
    -> std::expected<file, std::error_code> {
  const auto desired_access = pr::try_get_file_access(options.access);

  if (not desired_access) {
    return std::unexpected{desired_access.error()};
  }

  const auto creation_disposition = pr::try_get_file_creation(options.creation);

  if (not creation_disposition) {
    return std::unexpected{creation_disposition.error()};
  }

  const auto handle = CreateFileW(
      path.c_str(), *desired_access,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      *creation_disposition, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    return std::unexpected{pr::last_error()};
  }

  return file{handle};
}

auto pr::try_set_length(const file &fd, std::int64_t length) noexcept
    -> std::expected<void, std::error_code> {
  FILE_END_OF_FILE_INFO info{.EndOfFile = {.QuadPart = length}};

  if (SetFileInformationByHandle(fd.native_handle(), FileEndOfFileInfo,
                                 std::addressof(info), sizeof(info)) == FALSE) {
    return std::unexpected{pr::last_error()};
  }

  return {};
}

auto pr::try_seek_end(const file &fd) noexcept
    -> std::expected<std::int64_t, std::error_code> {
  const LARGE_INTEGER distance{};
  LARGE_INTEGER position{};

  if (SetFilePointerEx(fd.native_handle(), distance, std::addressof(position),
                       FILE_END) == FALSE) {
    return std::unexpected{pr::last_error()};
  }

  return position.QuadPart;
}
