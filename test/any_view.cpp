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

// https://en.cppreference.com/w/cpp/ranges#simple-view
template <class R>
concept simple_view =
    std::ranges::view<R> && std::ranges::range<const R> &&
    std::same_as<std::ranges::iterator_t<R>,
                 std::ranges::iterator_t<const R>> &&
    std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<const R>>;

auto main() -> int {
  using enum pr::any_kind;
  using pr::any_iterator;
  using pr::any_view;

  // clang-format off
  static_assert(std::constructible_from<
      any_view<int, contiguous | borrowed | constant | copyable | sized>,
      std::span<int>>);
  // cannot obtain int & from std::span<const int>
  static_assert(not std::constructible_from<
      any_view<int>,
      std::span<const int>>);
  // can obtain const int & from std::span<const int>
  static_assert(std::constructible_from<
      any_view<const int>,
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
  // cannot obtain int & from const std::vector<int>
  static_assert(not std::constructible_from<
      any_view<int, constant>,
      std::vector<int>>);
  // can obtain const int & from const std::vector<int>
  static_assert(std::constructible_from<
      any_view<const int, constant>,
      std::vector<int>>);
  // cannot borrow from std::ranges::owning_view<std::vector<int>>
  static_assert(not std::constructible_from<
      any_view<int, borrowed>,
      std::vector<int>>);
  // can borrow from std::ranges::ref_view<std::vector<int>>
  static_assert(std::constructible_from<
      any_view<int, borrowed>,
      std::vector<int> &>);
  // cannot copy std::ranges::owning_view<std::vector<int>>
  static_assert(not std::constructible_from<
      any_view<int, copyable>,
      std::vector<int>>);
  // can copy std::ranges::ref_view<std::vector<int>>
  static_assert(std::constructible_from<
      any_view<int, copyable>,
      std::vector<int> &>);
  static_assert(std::constructible_from<
      any_view<bool, random_access | borrowed | constant | copyable | sized, bool>,
      std::vector<bool> &>);
  // std::vector<bool> specialization is not a contiguous range
  static_assert(not std::constructible_from<
      any_view<bool, contiguous, bool>,
      std::vector<bool> &>);
  // std::vector<bool> specialization is a random access range
  static_assert(std::constructible_from<
      any_view<bool, random_access, bool>,
      std::vector<bool>>);
  // cannot obtain bool & from std::vector<bool>
  static_assert(not std::constructible_from<
      any_view<bool>,
      std::vector<bool>>);
  // can obtain bool from std::vector<bool>
  static_assert(std::constructible_from<
      any_view<bool, input, bool>,
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
  // cannot obtain int & from std::set<int>
  static_assert(not std::constructible_from<
      any_view<int>,
      std::set<int>>);
  // can obtain const int & from std::set<int>
  static_assert(std::constructible_from<
      any_view<const int>,
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
      any_view<int, forward>,
      std::ranges::istream_view<int>>);
  // std::ranges::istream_view<int> is an input range
  static_assert(std::constructible_from<
      any_view<int, input>,
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
      any_view<float, copyable, float>,
      transform<std::vector<int>, float>>);
  // std::ranges::ref_view<std::vector<int>> is copyable
  static_assert(std::constructible_from<
      any_view<float, copyable, float>,
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
  static_assert(std::ranges::range<const any_view<int, simple>>);

  static_assert(not simple_view<any_view<int>>);
  static_assert(not simple_view<any_view<int, contiguous>>);
  static_assert(not simple_view<any_view<int, borrowed>>);
  static_assert(not simple_view<any_view<int, copyable>>);
  static_assert(not simple_view<any_view<int, sized>>);
  static_assert(simple_view<any_view<int, constant>>);
  static_assert(simple_view<any_view<int, simple>>);
  static_assert(constant == simple);

  static_assert(not std::ranges::sized_range<any_view<int>>);
  static_assert(not std::ranges::sized_range<any_view<int, contiguous>>);
  static_assert(not std::ranges::sized_range<any_view<int, borrowed>>);
  static_assert(not std::ranges::sized_range<any_view<int, copyable>>);
  static_assert(not std::ranges::sized_range<any_view<int, constant>>);
  static_assert(std::ranges::sized_range<any_view<int, sized>>);

  static_assert(not std::ranges::common_range<any_view<int, random_access>>);
  static_assert(not std::ranges::common_range<any_view<int, bidirectional | sized>>);
  static_assert(std::ranges::common_range<any_view<int, random_access | sized>>);
  static_assert(std::ranges::common_range<any_view<int, common>>);
  static_assert((random_access | sized) == common);

  static_assert(std::same_as<
      std::ranges::borrowed_iterator_t<any_view<int>>,
      std::ranges::dangling>);
  static_assert(std::same_as<
      std::ranges::borrowed_iterator_t<any_view<int> &>,
      any_iterator<int>>);
  static_assert(std::same_as<
      std::ranges::borrowed_iterator_t<any_view<int, borrowed>>,
      any_iterator<int, borrowed>>);
  // clang-format on

  constexpr auto is_forward = []<class T>(T value) -> bool {
    using iterator = std::ranges::iterator_t<T>;
    constexpr auto enabled = std::forward_iterator<iterator>;

    if constexpr (enabled) {
      // view_interface should enable empty for forward range
      return value.empty()
             // value initialized forward iterators should compare equal
             and iterator() == iterator();
    }

    return enabled;
  };

  static_assert(not is_forward(any_view<int>()));
  static_assert(is_forward(any_view<int, forward>()));

  constexpr auto is_bidirectional = []<class T>(T value) -> bool {
    constexpr auto enabled = std::ranges::bidirectional_range<T>;

    if constexpr (enabled) {
      auto it1 = ++std::ranges::begin(value);
      auto it2 = it1;
      auto it3 = it2--;
      // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
      return it1 == it3 and --it1 == it2;
    }

    return enabled;
  };

  // clang-format off
  static_assert(not is_bidirectional(any_view<int, forward>(std::vector{42})));
  static_assert(is_bidirectional(any_view<int, bidirectional>(std::vector{42})));
  // clang-format on

  constexpr auto is_common = []<class T>(T value) -> bool {
    constexpr auto enabled = std::ranges::common_range<T>;

    if constexpr (enabled) {
      // view_interface should enable back for bidirectional common range
      return value.back() == 42;
    }

    return enabled;
  };

  // clang-format off
  static_assert(not is_common(any_view<int, random_access>(std::vector{42})));
  static_assert(not is_common(any_view<int, bidirectional | sized>(std::vector{42})));
  static_assert(is_common(any_view<int, common>(std::vector{42})));
  // clang-format on

  constexpr auto can_borrow = []<class T>(T value) -> bool {
    constexpr auto enabled = std::ranges::borrowed_range<T>;

    if constexpr (enabled) {
      // borrow iterator from temporary range
      const auto begin = std::ranges::begin(static_cast<T>(std::move(value)));
      return *begin == 42;
    }

    return enabled;
  };

  // clang-format off
  static_assert(not can_borrow(any_view<const int>(std::span<const int>({42}))));
  static_assert(can_borrow(any_view<const int, borrowed>(std::span<const int>({42}))));
  // clang-format on

  constexpr auto can_copy = []<class T>(T value) -> bool {
    constexpr auto enabled = std::copy_constructible<T>;

    if constexpr (enabled) {
      const auto other = value;
    }

    return enabled;
  };

  static_assert(not can_copy(any_view<int>()));
  static_assert(can_copy(any_view<int, copyable>()));

  constexpr auto is_sized = []<class T>(T value) -> bool {
    constexpr auto enabled = std::ranges::sized_range<T>;

    if constexpr (enabled) {
      return value.size() == 0;
    }

    return enabled;
  };

  static_assert(not is_sized(any_view<int>()));
  static_assert(is_sized(any_view<int, sized>()));
}
