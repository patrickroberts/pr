#include "aliases.hpp"

#include <pr/charconv.hpp>
#include <pr/file.hpp>
#include <pr/mapping.hpp>
#include <pr/result.hpp>

#include <cstdio>
#include <cstdlib>
#include <memory_resource>
#include <new>
#include <print>

namespace {

auto run(pr::args args) -> std::expected<void, std::error_code> {
  const auto offset = PR_MAYBE(pr::try_from_chars<std::ptrdiff_t>(args[2]));
  const auto file = PR_MAYBE(pr::file::try_open(args[1], O_RDWR));
  const auto bytes = PR_MAYBE(pr::try_lseek(file, 0, SEEK_END));
  const auto view = PR_MAYBE(pr::mapping::try_mmap(
      nullptr, bytes, {.flags = MAP_SHARED, .fd = file.native_handle()}));
  auto *const base = std::next(view.data(), offset);

  pr::with_context(*std::pmr::null_memory_resource(), [&] -> auto {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *const ptr = std::launder(reinterpret_cast<pr::vector<int> *>(base));
    std::println("{}", *ptr);
  });

  return {};
}

} // namespace

auto main(int argc, char **argv) -> int {
  if (argc != 3) {
    std::println(stderr, "Usage: {} [input_file] [base_offset]", *argv);
    return EXIT_FAILURE;
  }

  if (const auto result = run({argv, static_cast<std::size_t>(argc)});
      not result) {
    std::println(stderr, "{}", result.error().message());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
