#include "aliases.hpp"

#include <pr/context_allocator.hpp>
#include <pr/file.hpp>
#include <pr/mapping.hpp>

#include <cstdio>
#include <cstdlib>
#include <print>

auto main(int argc, char **argv) -> int {
  constexpr int bytes = 1024;
  constexpr int values = (bytes - sizeof(pr::vector<int>)) / sizeof(int);

  const std::span<char *> arguments{argv, static_cast<std::size_t>(argc)};

  if (arguments.size() != 2) {
    std::println(stderr, "Usage: {} [output_file]", arguments[0]);
    return EXIT_FAILURE;
  }

  auto maybe_file = pr::file::try_open(arguments[1], O_CREAT | O_RDWR,
                                       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (not maybe_file) {
    std::println(stderr, "{}", maybe_file.error().message());
    return EXIT_FAILURE;
  }

  const auto file = *std::move(maybe_file);
  ftruncate(*file, bytes);
  auto maybe_view = pr::mapping::try_mmap(
      nullptr, bytes,
      {.prot = PROT_READ | PROT_WRITE, .flags = MAP_SHARED, .fd = *file});

  if (not maybe_view) {
    std::println(stderr, "{}", maybe_view.error().message());
    return EXIT_FAILURE;
  }

  const auto view = *std::move(maybe_view);
  auto resource = std::pmr::monotonic_buffer_resource{
      view.data(),
      static_cast<std::size_t>(view.size()), // LWG-3646
      std::pmr::null_memory_resource()};

  auto *const ptr = pr::with_context(resource, [&] -> auto {
    pr::context_allocator<pr::vector<int>> alloc;
    return std::construct_at(alloc.allocate(1), std::from_range,
                             std::views::iota(0, values));
  });

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  std::println("{}", reinterpret_cast<std::byte *>(ptr) - view.data());

  return EXIT_SUCCESS;
}
