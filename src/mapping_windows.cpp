#include "last_error_windows.hpp"
#include "unexpected.hpp"

#include <pr/mapping.hpp>
#include <pr/result.hpp>

#include <Windows.h>

#include <utility>

namespace pr {
namespace {

inline auto try_get_mapping_protection(mapping::access_mode access,
                                       mapping::sharing_mode sharing)
    -> std::expected<DWORD, std::error_code> {
  using enum mapping::access_mode;

  switch (access) {
  case read:
    return PAGE_READONLY;
  case write:
  case read_write:
    using enum mapping::sharing_mode;

    switch (sharing) {
    case shared:
      return PAGE_READWRITE;
    case private_copy:
      return PAGE_WRITECOPY;
    }
  }

  return pr::unexpected(std::errc::invalid_argument);
}

inline auto get_mapping_desired_access(DWORD protection) noexcept -> DWORD {
  switch (protection) {
  case PAGE_READONLY:
    return FILE_MAP_READ;
  case PAGE_READWRITE:
    return FILE_MAP_WRITE;
  case PAGE_WRITECOPY:
    return FILE_MAP_COPY;
  }

  std::unreachable();
}

inline auto get_file_handle_and_max_size(const file *handle, std::size_t len)
    -> std::pair<HANDLE, std::uint64_t> {
  if (handle != nullptr) {
    return {handle->native_handle(), 0};
  }

  return {INVALID_HANDLE_VALUE, static_cast<std::uint64_t>(len)};
}

} // namespace
} // namespace pr

void pr::mapping::try_unmap() const noexcept {
  if (memory.empty()) {
    return;
  }

  UnmapViewOfFile(memory.data());
}

auto pr::mapping::try_mmap(void *addr, std::size_t len,
                           configuration config) noexcept
    -> std::expected<mapping, std::error_code> {
  if (len == 0) {
    return pr::unexpected(std::errc::invalid_argument);
  }

  if (not std::in_range<std::uint64_t>(len)) {
    return pr::unexpected(std::errc::value_too_large);
  }

  const auto protection =
      pr::try_get_mapping_protection(config.access, config.sharing);

  if (not protection) {
    return std::unexpected{protection.error()};
  }

  if (config.handle == nullptr and config.offset != 0) {
    return pr::unexpected(std::errc::invalid_argument);
  }

  constexpr auto dword_bits = 32U;
  const auto [file_handle, max_size] =
      pr::get_file_handle_and_max_size(config.handle, len);
  const auto mapping_handle =
      CreateFileMappingW(file_handle, nullptr, *protection,
                         static_cast<DWORD>(max_size >> dword_bits),
                         static_cast<DWORD>(max_size), nullptr);

  if (mapping_handle == nullptr) {
    return std::unexpected{pr::last_error()};
  }

  const auto desired_access = get_mapping_desired_access(*protection);
  addr = MapViewOfFile(mapping_handle, desired_access,
                       static_cast<DWORD>(config.offset >> dword_bits),
                       static_cast<DWORD>(config.offset), len);

  if (addr == nullptr) {
    const auto error = pr::last_error();
    CloseHandle(mapping_handle);
    return std::unexpected{error};
  }

  CloseHandle(mapping_handle);
  return mapping{static_cast<std::byte *>(addr), len};
}
