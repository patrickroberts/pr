#pragma once

#include <memory>
#include <ranges>

namespace pr {
namespace ranges {

template <class T>
concept copyable_view = std::copyable<T> and std::ranges::view<T>;

template <std::ranges::view ViewT>
  requires(not std::copyable<ViewT>)
class shared_view : public std::ranges::view_interface<shared_view<ViewT>> {
  std::shared_ptr<ViewT> base_;

public:
  shared_view()
    requires std::default_initializable<ViewT>
      : base_(std::make_shared<ViewT>()) {}

  explicit shared_view(ViewT base)
      : base_(std::make_shared<ViewT>(std::move(base))) {}

  auto begin() -> std::ranges::iterator_t<ViewT> {
    return std::ranges::begin(*base_);
  }

  auto end() -> std::ranges::sentinel_t<ViewT> {
    return std::ranges::end(*base_);
  }
};

template <std::ranges::viewable_range RangeT>
shared_view(RangeT &&) -> shared_view<std::views::all_t<RangeT>>;

namespace views {
namespace detail {

struct shared_fn {
  template <std::ranges::viewable_range RangeT>
  constexpr auto operator()(RangeT &&range) const -> copyable_view auto {
    if constexpr (std::copyable<std::views::all_t<RangeT>>) {
      return std::views::all(std::forward<RangeT>(range));
    } else {
      return shared_view{std::forward<RangeT>(range)};
    }
  }
};

} // namespace detail

inline constexpr detail::shared_fn shared{};

} // namespace views
} // namespace ranges

namespace views = ranges::views;

} // namespace pr

template <class T>
inline constexpr bool
    std::ranges::enable_borrowed_range<pr::ranges::shared_view<T>> =
        std::ranges::enable_borrowed_range<T>;
