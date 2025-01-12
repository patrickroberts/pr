#pragma once

#include <pr/any_kind.hpp>

#include <ranges>

namespace pr::detail {

consteval auto enables_kind(any_kind mask, any_kind flag) -> bool {
  return (mask & flag) == flag;
}

template <any_kind MaskV, any_kind FlagV>
concept disables_kind = not detail::enables_kind(MaskV, FlagV);

template <class IteratorT, any_kind KindV>
concept models_input_iterator = std::input_iterator<IteratorT>;

template <class IteratorT, any_kind KindV>
concept disables_or_models_forward_iterator =
    models_input_iterator<IteratorT, KindV> and
    (disables_kind<KindV, any_kind::forward> or
     std::forward_iterator<IteratorT>);

template <class IteratorT, any_kind KindV>
concept disables_or_models_bidirectional_iterator =
    disables_or_models_forward_iterator<IteratorT, KindV> and
    (disables_kind<KindV, any_kind::bidirectional> or
     std::bidirectional_iterator<IteratorT>);

template <class IteratorT, any_kind KindV>
concept disables_or_models_random_access_iterator =
    disables_or_models_bidirectional_iterator<IteratorT, KindV> and
    (disables_kind<KindV, any_kind::random_access> or
     std::random_access_iterator<IteratorT>);

template <class IteratorT, any_kind KindV>
concept disables_or_models_contiguous_iterator =
    disables_or_models_random_access_iterator<IteratorT, KindV> and
    (disables_kind<KindV, any_kind::contiguous> or
     std::contiguous_iterator<IteratorT>);

template <class From, class To>
concept common_reference_same_as =
    std::same_as<std::common_reference_t<From, To>, To>;

template <class IteratorT, any_kind KindV, class ReferenceT, class SentinelT>
concept models_iterator_of =
    disables_or_models_contiguous_iterator<IteratorT, KindV> and
    common_reference_same_as<std::iter_reference_t<IteratorT>, ReferenceT> and
    std::sentinel_for<SentinelT, IteratorT>;

template <class RangeT, any_kind KindV>
concept disables_or_models_borrowed_range =
    disables_kind<KindV, any_kind::borrowed> or
    std::ranges::borrowed_range<RangeT>;

template <class RangeT, any_kind KindV>
concept disables_or_models_sized_range =
    disables_kind<KindV, any_kind::sized> or std::ranges::sized_range<RangeT>;

template <class MaybeConstantRangeT, any_kind KindV, class ReferenceT>
concept models_range_of =
    std::ranges::range<MaybeConstantRangeT> and
    models_iterator_of<std::ranges::iterator_t<MaybeConstantRangeT>, KindV,
                       ReferenceT,
                       std::ranges::sentinel_t<MaybeConstantRangeT>> and
    disables_or_models_borrowed_range<MaybeConstantRangeT, KindV> and
    disables_or_models_sized_range<MaybeConstantRangeT, KindV>;

template <class RangeT, any_kind KindV>
concept disables_or_models_copyable_range =
    disables_kind<KindV, any_kind::copyable> or std::copyable<RangeT>;

template <class ViewT, any_kind KindV>
using const_if_enabled_t =
    std::conditional_t<detail::enables_kind(KindV, any_kind::constant),
                       const ViewT, ViewT>;

template <class ViewT, any_kind KindV, class ReferenceT>
concept models_view_of =
    std::ranges::view<ViewT> and
    models_range_of<const_if_enabled_t<ViewT, KindV>, KindV, ReferenceT> and
    disables_or_models_copyable_range<ViewT, KindV>;

template <class RangeT, any_kind KindV, class ReferenceT>
concept models_viewable_range_of =
    std::ranges::viewable_range<RangeT> and
    models_view_of<std::views::all_t<RangeT>, KindV, ReferenceT>;

template <class T1, class T2>
inline constexpr bool is_instantiation_of_v = false;

template <class ElementT1, auto KindV1, class ReferenceT1, class DifferenceT1,
          class ElementT2, auto KindV2, class ReferenceT2, class DifferenceT2,
          template <class, auto, class, class> class AnyViewT>
inline constexpr bool is_instantiation_of_v<
    AnyViewT<ElementT1, KindV1, ReferenceT1, DifferenceT1>,
    AnyViewT<ElementT2, KindV2, ReferenceT2, DifferenceT2>> = true;

template <class RangeT, class AnyViewT>
concept convertible_to_any_view =
    not is_instantiation_of_v<std::decay_t<RangeT>, AnyViewT> and
    models_viewable_range_of<RangeT, AnyViewT::kind,
                             std::ranges::range_reference_t<AnyViewT>>;

template <class InterfaceT>
concept interface_copyable =
    requires(const InterfaceT &instance, void *destination) {
      { instance.copy_to(destination) } -> std::same_as<void>;
      { instance.copy() } -> std::same_as<InterfaceT *>;
    };

} // namespace pr::detail
