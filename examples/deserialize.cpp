#include "aliases.hpp"

#include <pr/file.hpp>
#include <pr/mapping.hpp>

#include <cstdio>
#include <cstdlib>
#include <memory_resource>
#include <new>
#include <print>
#include <span>

auto main(int argc, char **argv) -> int {
  const std::span<char *> arguments{argv, static_cast<std::size_t>(argc)};

  if (arguments.size() != 3) {
    std::println(stderr, "Usage: {} [input_file] [base_offset]", arguments[0]);
    return EXIT_FAILURE;
  }

  const auto file = pr::file::try_open(arguments[1], O_RDWR).value();
  const auto bytes = lseek(*file, 0, SEEK_END);

  if (bytes == -1) {
    return EXIT_FAILURE;
  }

  auto maybe_view = pr::mapping::try_mmap(
      nullptr, bytes,
      {.prot = PROT_READ | PROT_WRITE, .flags = MAP_SHARED, .fd = *file});

  if (not maybe_view) {
    std::println(stderr, "{}", maybe_view.error().message());
    return EXIT_FAILURE;
  }

  const auto view = std::move(maybe_view).value();

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  auto *const base = view.data() + std::strtoull(arguments[2], nullptr, 10);

  pr::with_context(*std::pmr::null_memory_resource(), [&] {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *const ptr = reinterpret_cast<pr::vector<int> *>(base);
    std::println("{}", *ptr);
  });

  return EXIT_SUCCESS;
}
