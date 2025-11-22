#pragma once

#include <memory>
#include <ranges>

namespace pr {
namespace ranges {

template <class T>
concept copyable_view = std::ranges::view<T> and std::copyable<T>;

template <class T>
concept shared_range =
    std::ranges::viewable_range<T> and std::copyable<std::views::all_t<T>>;

template <std::ranges::viewable_range R>
  requires std::movable<R>
class shared_view : public std::ranges::view_interface<shared_view<R>> {
  std::shared_ptr<R> range_ptr;

public:
  shared_view()
    requires std::default_initializable<R>
      : range_ptr(std::make_shared<R>()) {}

  explicit shared_view(R &&range)
      : range_ptr(std::make_shared<R>(std::move(range))) {}

  [[nodiscard]] auto base() const noexcept -> R & { return *range_ptr; }

  [[nodiscard]] auto begin() const
      noexcept(noexcept(std::ranges::begin(*range_ptr)))
          -> std::ranges::iterator_t<R> {
    return std::ranges::begin(*range_ptr);
  }

  [[nodiscard]] auto end() const
      noexcept(noexcept(std::ranges::end(*range_ptr)))
          -> std::ranges::sentinel_t<R> {
    return std::ranges::end(*range_ptr);
  }
};

namespace views {
namespace detail {

struct shared_fn : std::ranges::range_adaptor_closure<shared_fn> {
  template <std::ranges::viewable_range R>
    requires shared_range<R> or std::movable<R>
  [[nodiscard]] constexpr auto operator()(R &&range) const
      noexcept(shared_range<R> and
               noexcept(std::views::all(std::forward<R>(range))))
          -> copyable_view auto {
    if constexpr (shared_range<R>) {
      return std::views::all(std::forward<R>(range));
    } else {
      return shared_view{std::forward<R>(range)};
    }
  }
};

} // namespace detail

inline constexpr detail::shared_fn shared{};

template <std::ranges::viewable_range R>
  requires shared_range<R> or std::movable<R>
using shared_t = decltype(views::shared(std::declval<R>()));

} // namespace views
} // namespace ranges

namespace views = ranges::views;

} // namespace pr

template <class T>
inline constexpr bool
    std::ranges::enable_borrowed_range<pr::ranges::shared_view<T>> =
        std::ranges::enable_borrowed_range<T>;
