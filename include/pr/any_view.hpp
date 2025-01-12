#pragma once

#include <pr/detail/view_adaptor.hpp>

#include <type_traits>

namespace pr {

template <class ElementT, any_kind KindV = any_kind::input,
          class ReferenceT = std::add_lvalue_reference_t<ElementT>,
          class DifferenceT = std::ptrdiff_t>
class any_view : public std::ranges::view_interface<
                     any_view<ElementT, KindV, ReferenceT, DifferenceT>> {
  using iterator_ = any_iterator<ElementT, KindV & any_kind::contiguous,
                                 ReferenceT, DifferenceT>;

  template <class RangeT>
  static constexpr std::in_place_type_t<detail::view_adaptor<
      ElementT, KindV, ReferenceT, DifferenceT, std::views::all_t<RangeT>>>
      in_place_view_type_{};

  static constexpr bool constant_ =
      detail::enables_kind(KindV, any_kind::constant);
  static constexpr bool copyable_ =
      detail::enables_kind(KindV, any_kind::copyable);
  static constexpr bool sized_ = detail::enables_kind(KindV, any_kind::sized);

  // inplace storage is large enough for vtable pointer and std::vector<T>
  using view_ptr_type_ = detail::intrusive_small_ptr<
      detail::view_interface<ElementT, KindV, ReferenceT, DifferenceT>,
      void *[4]>;

  view_ptr_type_ view_ptr_;

  using value_type_ = std::remove_cv_t<ElementT>;
  using size_type_ = std::make_unsigned_t<DifferenceT>;

public:
  static constexpr any_kind kind = KindV;

  template <detail::convertible_to_any_view<any_view> RangeT>
  constexpr any_view(RangeT &&range)
      : view_ptr_(in_place_view_type_<RangeT>,
                  std::views::all(static_cast<RangeT &&>(range))) {}

  constexpr any_view()
      : any_view(std::views::empty<std::remove_reference_t<ReferenceT>>) {}

  any_view(const any_view &other) = delete;

  constexpr any_view(const any_view &other)
    requires copyable_
  = default;

  constexpr any_view(any_view &&other) noexcept = default;

  auto operator=(const any_view &other) -> any_view & = delete;

  constexpr auto operator=(const any_view &other) -> any_view &
    requires copyable_
  = default;

  constexpr auto operator=(any_view &&other) noexcept -> any_view & = default;

  [[nodiscard]] constexpr auto begin() -> iterator_
    requires(not constant_)
  {
    return view_ptr_->begin();
  }

  [[nodiscard]] constexpr auto begin() const -> iterator_
    requires constant_
  {
    return view_ptr_->begin();
  }

  [[nodiscard]] constexpr auto end() const -> std::default_sentinel_t {
    return std::default_sentinel;
  }

  [[nodiscard]] constexpr auto size() const -> size_type_
    requires sized_
  {
    return view_ptr_->size();
  }
};

} // namespace pr

template <class ElementT, pr::any_kind KindV, class ReferenceT,
          class DifferenceT>
inline constexpr bool std::ranges::enable_borrowed_range<
    pr::any_view<ElementT, KindV, ReferenceT, DifferenceT>> =
    pr::detail::enables_kind(KindV, pr::any_kind::borrowed);
