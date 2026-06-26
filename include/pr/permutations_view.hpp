#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <type_traits>
#include <utility>

namespace pr {
namespace ranges {

template <std::ranges::view V, class Comp = std::ranges::less,
          class Proj = std::identity>
  requires std::ranges::random_access_range<V> and
           std::sortable<std::ranges::iterator_t<V>, Comp, Proj> and
           std::movable<Comp> and std::movable<Proj>
class permutations_view
    : public std::ranges::view_interface<permutations_view<V, Comp, Proj>> {

  [[no_unique_address]] V base_;
  [[no_unique_address]] Comp comp_;
  [[no_unique_address]] Proj proj_;
  bool sorted_{false};

  class iterator {
    friend permutations_view;

    permutations_view *parent_;
    bool found_{true};

    constexpr explicit iterator(permutations_view *parent) : parent_(parent) {}

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = V;
    using reference = V &;

    iterator(const iterator &) = delete;
    iterator(iterator &&) = default;

    auto operator=(const iterator &) -> iterator & = delete;
    auto operator=(iterator &&) -> iterator & = default;

    ~iterator() = default;

    [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
      return parent_->base_;
    }

    constexpr auto operator++() -> iterator & {
      found_ = std::ranges::next_permutation(parent_->base_,
                                             std::ref(parent_->comp_),
                                             std::ref(parent_->proj_))
                   .found;
      parent_->sorted_ = not found_;
      return *this;
    }

    constexpr void operator++(int) { operator++(); }
  };

  struct sentinel {
    [[nodiscard]] constexpr auto operator==(const iterator &it) const noexcept
        -> bool {
      return not it.found_;
    }
  };

public:
  permutations_view()
    requires std::default_initializable<V> and
                 std::default_initializable<Comp> and
                 std::default_initializable<Proj>
  = default;

  constexpr explicit permutations_view(V base, Comp comp = Comp(),
                                       Proj proj = Proj())
      : base_(std::move(base)), comp_(std::move(comp)), proj_(std::move(proj)) {
  }

  constexpr explicit permutations_view(
      [[maybe_unused]] std::sorted_equivalent_t tag, V base, Comp comp = Comp(),
      Proj proj = Proj())
      : base_(std::move(base)), comp_(std::move(comp)), proj_(std::move(proj)),
        sorted_(true) {}

  [[nodiscard]] constexpr auto base() const & noexcept -> V
    requires std::copy_constructible<V>
  {
    return base_;
  }

  [[nodiscard]] constexpr auto base() && noexcept -> V {
    return std::move(base_);
  }

  [[nodiscard]] constexpr auto begin() -> iterator {
    if (not sorted_) {
      std::ranges::sort(base_, std::ref(comp_), std::ref(proj_));
      sorted_ = true;
    }

    return iterator{this};
  }

  [[nodiscard]] constexpr auto end() -> sentinel { return sentinel{}; };
};

template <class R, class Comp, class Proj>
permutations_view(R &&, Comp, Proj)
    -> permutations_view<std::views::all_t<R>, Comp, Proj>;

template <class R, class Comp, class Proj>
permutations_view(std::sorted_equivalent_t, R &&, Comp, Proj)
    -> permutations_view<std::views::all_t<R>, Comp, Proj>;

template <class R, class Comp, class Proj>
concept permutable_range =
    std::ranges::viewable_range<R> and
    std::constructible_from<
        permutations_view<std::views::all_t<R>, std::decay_t<Comp>,
                          std::decay_t<Proj>>,
        R, Comp, Proj>;

namespace views {

struct permutations_fn {
  template <class R, class Comp = std::ranges::less, class Proj = std::identity>
    requires permutable_range<R, Comp, Proj>
  [[nodiscard]] constexpr auto operator()(R &&r, Comp &&comp = Comp(),
                                          Proj &&proj = Proj()) const {
    return permutations_view(std::forward<R>(r), std::forward<Comp>(comp),
                             std::forward<Proj>(proj));
  }

  template <class R, class Comp = std::ranges::less, class Proj = std::identity>
    requires permutable_range<R, Comp, Proj>
  [[nodiscard]] constexpr auto operator()(std::sorted_equivalent_t tag, R &&r,
                                          Comp &&comp = Comp(),
                                          Proj &&proj = Proj()) const {
    return permutations_view(tag, std::forward<R>(r), std::forward<Comp>(comp),
                             std::forward<Proj>(proj));
  }

  template <class Comp = std::ranges::less, class Proj = std::identity>
  [[nodiscard]] constexpr auto operator()(Comp &&comp = Comp(),
                                          Proj &&proj = Proj()) const {
    return bound_fn<Comp, Proj, false>{
        .comp_ = std::forward<Comp>(comp),
        .proj_ = std::forward<Proj>(proj),
    };
  }

  template <class Comp = std::ranges::less, class Proj = std::identity>
  [[nodiscard]] constexpr auto
  operator()([[maybe_unused]] std::sorted_equivalent_t tag,
             Comp &&comp = Comp(), Proj &&proj = Proj()) const {
    return bound_fn<Comp, Proj, true>{
        .comp_ = std::forward<Comp>(comp),
        .proj_ = std::forward<Proj>(proj),
    };
  }

private:
  template <class Comp, class Proj, bool Sorted>
  struct bound_fn
      : std::ranges::range_adaptor_closure<bound_fn<Comp, Proj, Sorted>> {
    [[no_unique_address]] Comp comp_;
    [[no_unique_address]] Proj proj_;

    template <class Self, permutable_range<Comp, Proj> R>
    [[nodiscard]] constexpr auto operator()(this Self &&self, R &&r) {
      if constexpr (Sorted) {
        return permutations_view(std::sorted_equivalent, std::forward<R>(r),
                                 std::forward<Self>(self).comp_,
                                 std::forward<Self>(self).proj_);
      } else {
        return permutations_view(std::forward<R>(r),
                                 std::forward<Self>(self).comp_,
                                 std::forward<Self>(self).proj_);
      }
    }
  };
};

inline constexpr permutations_fn permutations{};

} // namespace views
} // namespace ranges

namespace views = ranges::views;

} // namespace pr
