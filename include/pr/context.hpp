#pragma once

#include <utility>

namespace pr {
namespace detail {

template <class T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static thread_local std::add_pointer_t<T> context_ = nullptr;

template <class T>
class context {
  [[no_unique_address]] T active_;
  std::add_pointer_t<T> previous_;

public:
  template <class... ArgsT>
    requires std::constructible_from<T, ArgsT...>
  explicit context(ArgsT &&...args) noexcept(
      std::is_nothrow_constructible_v<T, ArgsT...>)
      : active_(std::forward<ArgsT>(args)...),
        previous_(std::exchange(context_<T>, std::addressof(active_))) {}

  context(const context &) = delete;

  context(context &&) = delete;

  ~context() noexcept { context_<T> = previous_; }
};

} // namespace detail

template <class T, class... ArgsT>
  requires std::constructible_from<T, ArgsT...>
[[nodiscard]] auto make_context(ArgsT &&...args) noexcept(
    std::is_nothrow_constructible_v<T, ArgsT...>) {
  return detail::context<T>(std::forward<ArgsT>(args)...);
}

template <class T>
[[nodiscard]] auto get_context() noexcept -> std::add_pointer_t<T> {
  return detail::context_<T>;
}

} // namespace pr
