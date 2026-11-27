#include "last_error_posix.hpp"
#include "unexpected.hpp"

#include <pr/mapping.hpp>
#include <pr/result.hpp>

#include <sys/mman.h>

#include <utility>

namespace pr {
namespace {

inline auto try_get_mapping_access(mapping::access_mode access)
    -> std::expected<int, std::error_code> {
  using enum mapping::access_mode;

  switch (access) {
  case read:
    return PROT_READ;
  case write:
    return PROT_WRITE;
  case read_write:
    return PROT_READ | PROT_WRITE;
  }

  return pr::unexpected(std::errc::invalid_argument);
}

inline auto try_get_mapping_sharing(mapping::sharing_mode sharing)
    -> std::expected<int, std::error_code> {
  using enum mapping::sharing_mode;

  switch (sharing) {
  case shared:
    return MAP_SHARED;
  case private_copy:
    return MAP_PRIVATE;
  }

  return pr::unexpected(std::errc::invalid_argument);
}

inline auto get_mapping_handle(const file *handle, int &flags) -> int {
  if (handle != nullptr) {
    return handle->native_handle();
  }

  // NOLINTNEXTLINE(bugprone-signed-bitwise)
  flags |= MAP_ANONYMOUS;
  return -1;
}

} // namespace
} // namespace pr

void pr::mapping::try_unmap() const noexcept {
  if (memory.empty()) {
    return;
  }

  munmap(memory.data(), memory.size());
}

auto pr::mapping::try_mmap(void *addr, std::size_t len,
                           configuration config) noexcept
    -> std::expected<mapping, std::error_code> {
  const auto protection = PR_MAYBE(pr::try_get_mapping_access(config.access));
  auto flags = PR_MAYBE(pr::try_get_mapping_sharing(config.sharing));

  if (not std::in_range<off_t>(config.offset)) {
    return pr::unexpected(std::errc::invalid_argument);
  }

  const auto fd = pr::get_mapping_handle(config.handle, flags);

  addr =
      mmap(addr, len, protection, flags, fd, static_cast<off_t>(config.offset));

  if (addr == MAP_FAILED) {
    return std::unexpected{pr::last_error()};
  }

  return mapping{static_cast<std::byte *>(addr), len};
}
