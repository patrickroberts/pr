#pragma once

#include <concepts>
#include <memory>
#include <memory_resource>
#include <utility>

namespace pr {
namespace detail {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local constinit std::pmr::memory_resource *resource = nullptr;

class context_exit {
  std::pmr::memory_resource *previous;

public:
  context_exit(std::pmr::memory_resource &current) noexcept
      : previous(std::exchange(resource, std::addressof(current))) {}

  context_exit(const context_exit &) = delete;
  context_exit(context_exit &&) = delete;

  auto operator=(const context_exit &) -> context_exit & = delete;
  auto operator=(context_exit &&) -> context_exit & = delete;

  ~context_exit() { resource = previous; }
};

} // namespace detail

template <std::invocable Fn>
auto with_context(std::pmr::memory_resource &resource,
                  Fn &&fn) noexcept(std::is_nothrow_invocable_v<Fn>)
    -> std::invoke_result_t<Fn> {
  const detail::context_exit restore{resource};
  return std::forward<Fn>(fn)();
}

inline auto get_resource() noexcept -> std::pmr::memory_resource * {
  return detail::resource;
}

template <class T = std::byte>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class context_allocator {
public:
  using value_type = T;

  context_allocator() = default;

  context_allocator(const context_allocator &other) = default;

  template <class U>
  constexpr context_allocator(const context_allocator<U> &other) noexcept {}

  [[nodiscard]] constexpr auto allocate(std::size_t n) -> value_type * {
    if (n > std::allocator_traits<context_allocator>::max_size(*this)) {
      throw std::bad_array_new_length();
    }

    // NOLINTNEXTLINE(readability-qualified-auto)
    if (const auto resource_ptr = get_resource()) {
      return static_cast<value_type *>(
          resource_ptr->allocate(n * sizeof(T), alignof(T)));
    }

    throw std::bad_alloc();
  }

  constexpr void deallocate(value_type *p, std::size_t n) {
    get_resource()->deallocate(p, n * sizeof(T), alignof(T));
  }

  [[nodiscard]] auto operator==(const context_allocator &other) const
      -> bool = default;

  template <class U>
  [[nodiscard]] constexpr auto
  operator==(const context_allocator<U> &other) const noexcept -> bool {
    return true;
  }
};

} // namespace pr
