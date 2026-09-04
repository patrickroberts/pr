#include "aliases.hpp"

#include <pr/charconv.hpp>
#include <pr/context_allocator.hpp>
#include <pr/file.hpp>
#include <pr/mapping.hpp>
#include <pr/result.hpp>

#include <cstdio>
#include <cstdlib>
#include <print>

namespace {

auto run(pr::args args) -> std::expected<void, std::error_code> {
  const auto values = PR_MAYBE(pr::try_from_chars<std::size_t>(args[2]));
  const auto bytes = sizeof(pr::vector<int>) + (values * sizeof(int));
  const auto file = PR_MAYBE(pr::file::try_open(
      args[1], O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

  PR_MAYBE(pr::try_ftruncate(file, bytes));

  const auto view = PR_MAYBE(pr::mapping::try_mmap(
      nullptr, bytes, {.flags = MAP_SHARED, .fd = file.native_handle()}));
  auto resource = std::pmr::monotonic_buffer_resource{
      view.data(),
      static_cast<std::size_t>(view.size()), // LWG-3646
      std::pmr::null_memory_resource()};

  // libstdc++'s monotonic_buffer_resource bumps upward while libc++'s bumps
  // downward. The container's location within the mapping is
  // implementation-dependent. Print the resulting offset rather than assuming a
  // fixed location.
  auto *const base = pr::with_context(resource, [&] -> auto {
    pr::context_allocator<pr::vector<int>> alloc;
    auto *const ptr =
        std::construct_at(alloc.allocate(1), std::from_range,
                          std::views::iota(0, static_cast<int>(values)));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<std::byte *>(ptr);
  });

  const auto offset = std::distance(view.data(), base);
  std::println("{}", offset);

  return {};
}

} // namespace

auto main(int argc, char **argv) -> int {
  if (argc != 3) {
    std::println(stderr, "Usage: {} [output_file] [values]", *argv);
    return EXIT_FAILURE;
  }

  if (const auto result = run({argv, static_cast<std::size_t>(argc)});
      not result) {
    std::println(stderr, "{}", result.error().message());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
