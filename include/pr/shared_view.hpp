#pragma once

#include <memory>
#include <ranges>

namespace pr {
namespace ranges {

template <class T>
concept copyable_view = std::ranges::view<T> and std::copyable<T>;

template <std::ranges::viewable_range RangeT>
  requires std::is_object_v<RangeT>
class shared_view : public std::ranges::view_interface<shared_view<RangeT>> {
  std::shared_ptr<RangeT> base_;

public:
  shared_view()
    requires std::default_initializable<RangeT>
      : base_(std::make_shared<RangeT>()) {}

  explicit shared_view(RangeT &&base)
      : base_(std::make_shared<RangeT>(std::move(base))) {}

  auto base() const noexcept -> RangeT & { return *base_; }

  auto begin() -> std::ranges::iterator_t<RangeT> {
    return std::ranges::begin(*base_);
  }

  auto end() -> std::ranges::sentinel_t<RangeT> {
    return std::ranges::end(*base_);
  }
};

namespace views {
namespace detail {

struct shared_fn {
  template <std::ranges::viewable_range RangeT>
    requires std::copyable<std::views::all_t<RangeT>> or
                 std::is_object_v<RangeT>
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
