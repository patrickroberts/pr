#include <pr/any_view.hpp>

#include <istream>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

template <std::ranges::viewable_range RangeT, class ReturnT>
using view_fn = ReturnT(std::ranges::range_reference_t<RangeT>);

template <std::ranges::viewable_range RangeT>
using filter = std::ranges::filter_view<std::views::all_t<RangeT>,
                                        view_fn<RangeT, bool> *>;

template <std::ranges::viewable_range RangeT, class ReturnT>
using transform = std::ranges::transform_view<std::views::all_t<RangeT>,
                                              view_fn<RangeT, ReturnT> *>;

auto main() -> int {
  using enum pr::any_kind;
  using pr::any_iterator;
  using pr::any_view;

  // clang-format off
  static_assert(std::constructible_from<
      any_view<int, contiguous | borrowed | constant | copyable | sized>,
      std::span<int>>);
  // cannot obtain int& from std::span<const int>
  static_assert(not std::constructible_from<
      any_view<int, contiguous | borrowed | constant | copyable | sized>,
      std::span<const int>>);
  static_assert(std::constructible_from<
      any_view<const int, contiguous | borrowed | constant | copyable | sized>,
      std::span<const int>>);
  static_assert(std::constructible_from<
      any_view<int, contiguous | borrowed | constant | copyable | sized>,
      std::vector<int> &>);
  static_assert(std::constructible_from<
      any_view<const int, contiguous | borrowed | constant | copyable | sized>,
      const std::vector<int> &>);
  static_assert(std::constructible_from<
      any_view<int, contiguous | sized>,
      std::vector<int>>);
  // cannot obtain int& from const std::vector<int>
  static_assert(not std::constructible_from<
      any_view<int, contiguous | constant | sized>,
      std::vector<int>>);
  static_assert(std::constructible_from<
      any_view<const int, contiguous | constant | sized>,
      std::vector<int>>);
  // cannot copy std::ranges::owning_view<std::vector<int>>
  static_assert(not std::constructible_from<
      any_view<int, contiguous | copyable | sized>,
      std::vector<int>>);
  static_assert(std::constructible_from<
      any_view<bool, random_access | borrowed | constant | copyable | sized, bool>,
      std::vector<bool> &>);
  // std::vector<bool> specialization is not a contiguous range
  static_assert(not std::constructible_from<
      any_view<bool, contiguous | borrowed | constant | copyable | sized, bool>,
      std::vector<bool> &>);
  static_assert(std::constructible_from<
      any_view<bool, random_access | sized, bool>,
      std::vector<bool>>);
  // cannot obtain bool& from std::vector<bool>
  static_assert(not std::constructible_from<
      any_view<bool, random_access | sized>,
      std::vector<bool>>);
  static_assert(std::constructible_from<
      any_view<int, bidirectional | sized>,
      std::list<int>>);
  static_assert(std::constructible_from<
      any_view<int, bidirectional | borrowed | constant | copyable | sized>,
      std::list<int> &>);
  static_assert(std::constructible_from<
      any_view<const int, bidirectional | constant | sized>,
      std::set<int>>);
  // cannot obtain int& from std::set<int>
  static_assert(not std::constructible_from<
      any_view<int, bidirectional | constant | sized>,
      std::set<int>>);
  static_assert(std::constructible_from<
      any_view<const int, bidirectional | borrowed | constant | copyable | sized>,
      std::set<int> &>);
  static_assert(std::constructible_from<
      any_view<const int, forward | sized>,
      std::unordered_set<int>>);
  static_assert(std::constructible_from<
      any_view<const int, forward | borrowed | constant | copyable | sized>,
      std::unordered_set<int> &>);
  static_assert(std::constructible_from<
      any_view<int, input | copyable>,
      std::ranges::istream_view<int>>);
  // std::ranges::istream_view<int> is not a forward range
  static_assert(not std::constructible_from<
      any_view<int, forward | copyable>,
      std::ranges::istream_view<int>>);
  static_assert(std::constructible_from<
      any_view<int, bidirectional | copyable>,
      filter<std::span<int>>>);
  static_assert(std::constructible_from<
      any_view<const int, bidirectional | copyable>,
      filter<std::span<const int>>>);
  static_assert(std::constructible_from<
      any_view<int, bidirectional>,
      filter<std::vector<int>>>);
  static_assert(std::constructible_from<
      any_view<int, bidirectional | copyable>,
      filter<std::vector<int> &>>);
  static_assert(std::constructible_from<
      any_view<const int, bidirectional | copyable>,
      filter<const std::vector<int> &>>);
  static_assert(std::constructible_from<
      any_view<float, random_access | sized, float>,
      transform<std::vector<int>, float>>);
  // std::ranges::owning_view<std::vector<int>> is not copyable
  static_assert(not std::constructible_from<
      any_view<float, random_access | copyable | sized, float>,
      transform<std::vector<int>, float>>);
  static_assert(std::constructible_from<
      any_view<float, random_access | copyable | sized, float>,
      transform<std::vector<int> &, float>>);

  static_assert(not std::ranges::input_range<any_view<void>>);
  static_assert(std::ranges::input_range<any_view<const int>>);
  static_assert(not std::ranges::output_range<any_view<const int>, int>);
  static_assert(std::ranges::output_range<any_view<int>, int>);
  static_assert(not std::ranges::forward_range<any_view<int>>);
  static_assert(std::ranges::forward_range<any_view<int, forward>>);
  static_assert(not std::ranges::bidirectional_range<any_view<int, forward>>);
  static_assert(std::ranges::bidirectional_range<any_view<int, bidirectional>>);
  static_assert(not std::ranges::random_access_range<any_view<int, bidirectional>>);
  static_assert(std::ranges::random_access_range<any_view<int, random_access>>);
  static_assert(not std::ranges::contiguous_range<any_view<int, random_access>>);
  static_assert(std::ranges::contiguous_range<any_view<int, contiguous>>);
  static_assert(not std::ranges::borrowed_range<any_view<int>>);
  static_assert(not std::ranges::borrowed_range<any_view<int, contiguous>>);
  static_assert(not std::ranges::borrowed_range<any_view<int, copyable>>);
  static_assert(not std::ranges::borrowed_range<any_view<int, constant>>);
  static_assert(not std::ranges::borrowed_range<any_view<int, sized>>);
  static_assert(std::ranges::borrowed_range<any_view<int, borrowed>>);
  static_assert(not std::copyable<any_view<int>>);
  static_assert(not std::copyable<any_view<int, contiguous>>);
  static_assert(not std::copyable<any_view<int, borrowed>>);
  static_assert(not std::copyable<any_view<int, constant>>);
  static_assert(not std::copyable<any_view<int, sized>>);
  static_assert(std::copyable<any_view<int, copyable>>);
  static_assert(not std::ranges::range<const any_view<int>>);
  static_assert(not std::ranges::range<const any_view<int, contiguous>>);
  static_assert(not std::ranges::range<const any_view<int, borrowed>>);
  static_assert(not std::ranges::range<const any_view<int, copyable>>);
  static_assert(not std::ranges::range<const any_view<int, sized>>);
  static_assert(std::ranges::range<const any_view<int, constant>>);
  static_assert(not std::ranges::sized_range<any_view<int>>);
  static_assert(not std::ranges::sized_range<any_view<int, contiguous>>);
  static_assert(not std::ranges::sized_range<any_view<int, borrowed>>);
  static_assert(not std::ranges::sized_range<any_view<int, copyable>>);
  static_assert(not std::ranges::sized_range<any_view<int, constant>>);
  static_assert(std::ranges::sized_range<any_view<int, sized>>);

  static_assert(std::same_as<
      std::ranges::borrowed_iterator_t<any_view<int>>,
      std::ranges::dangling>);
  static_assert(std::same_as<
      std::ranges::borrowed_iterator_t<any_view<int> &>,
      any_iterator<int>>);
  static_assert(std::same_as<
      std::ranges::borrowed_iterator_t<any_view<int, borrowed>>,
      any_iterator<int>>);
  // clang-format on
}
