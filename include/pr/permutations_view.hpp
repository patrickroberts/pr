#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <ranges>
#include <type_traits>
#include <utility>

namespace pr {
namespace ranges {

template <std::ranges::random_access_range R, class Comp = std::ranges::less,
          class Proj = std::identity>
  requires std::ranges::viewable_range<R> and
           std::sortable<std::ranges::iterator_t<R>, Comp, Proj> and
           std::is_object_v<Comp> and std::is_object_v<Proj>
class permutations_view
    : std::ranges::view_interface<permutations_view<R, Comp, Proj>> {
  using base_type = std::views::all_t<R>;

  [[no_unique_address]] base_type base_;
  [[no_unique_address]] Comp comp_;
  [[no_unique_address]] Proj proj_;

  struct sentinel {};

  class iterator {
    friend permutations_view;

    permutations_view *parent_;
    bool found{true};

    constexpr explicit iterator(permutations_view *parent) : parent_(parent) {}

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cvref_t<R>;
    using pointer = std::add_pointer_t<R>;
    using reference = R &;
    using iterator_category = std::input_iterator_tag;

    [[nodiscard]] constexpr auto
    operator==([[maybe_unused]] sentinel sent) const noexcept -> bool {
      return not found;
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
      return parent_->base();
    }

    constexpr auto operator++() -> iterator & {
      found = std::ranges::next_permutation(parent_->base(), parent_->comp(),
                                            parent_->proj())
                  .found;
      return *this;
    }

    constexpr auto operator++(int) -> void { operator++(); }
  };

  [[nodiscard]] constexpr auto proj() const noexcept -> const Proj & {
    return proj_;
  }

public:
  permutations_view()
    requires std::default_initializable<base_type> and
                 std::default_initializable<Comp> and
                 std::default_initializable<Proj>
  = default;

  // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
  constexpr explicit permutations_view(R &&base, Comp comp, Proj proj)
      : base_(std::forward<R>(base)), comp_(std::move(comp)),
        proj_(std::move(proj)) {
    std::ranges::sort(this->base(), this->comp(), this->proj());
  }

  [[nodiscard]] constexpr auto base() noexcept -> R & {
    if constexpr (std::same_as<std::remove_cvref_t<R>, base_type>) {
      return base_;
    } else {
      return base_.base();
    }
  }

  [[nodiscard]] constexpr auto comp() const noexcept -> const Comp & {
    return comp_;
  }

  [[nodiscard]] constexpr auto begin() -> iterator {
    assert(std::ranges::is_sorted(base(), comp(), proj()));

    return iterator{this};
  }

  [[nodiscard]] constexpr auto end() -> sentinel { return sentinel{}; };
};

template <class R, class Comp, class Proj>
permutations_view(R &&, Comp, Proj) -> permutations_view<R, Comp, Proj>;

template <class R, class Comp, class Proj>
concept permutation_viewable_range =
    std::ranges::viewable_range<R> and requires {
      permutations_view(std::declval<R>(), std::declval<Comp>(),
                        std::declval<Proj>());
    };

namespace views {

struct permutations_t {
  template <class R, class Comp = std::ranges::less, class Proj = std::identity>
    requires permutation_viewable_range<R, Comp, Proj>
  [[nodiscard]] constexpr auto operator()(R &&r, Comp &&comp = Comp(),
                                          Proj &&proj = Proj()) const {
    return permutations_view(std::forward<R>(r), std::forward<Comp>(comp),
                             std::forward<Proj>(proj));
  }

  template <class Comp = std::ranges::less, class Proj = std::identity>
  [[nodiscard]] constexpr auto operator()(Comp &&comp = Comp(),
                                          Proj &&proj = Proj()) const {
    return fn_t<Comp, Proj>{
        .comp_ = std::forward<Comp>(comp),
        .proj_ = std::forward<Proj>(proj),
    };
  }

private:
  template <class Comp, class Proj>
  struct fn_t : std::ranges::range_adaptor_closure<fn_t<Comp, Proj>> {
    [[no_unique_address]] Comp comp_;
    [[no_unique_address]] Proj proj_;

    template <class Self, permutation_viewable_range<Comp, Proj> R>
    [[nodiscard]] constexpr auto operator()(this Self &&self, R &&r) {
      return permutations_view(std::forward<R>(r),
                               std::forward<Self>(self).comp_,
                               std::forward<Self>(self).proj_);
    }
  };
};

inline constexpr permutations_t permutations{};

} // namespace views
} // namespace ranges

namespace views = ranges::views;

} // namespace pr
