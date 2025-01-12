#include <pr/any_iterator.hpp>

auto main() -> int {
  using enum pr::any_kind;
  using pr::any_iterator;

  // clang-format off
  static_assert(not std::input_iterator<any_iterator<void>>);
  static_assert(std::input_iterator<any_iterator<const int>>);
  static_assert(not std::output_iterator<any_iterator<const int>, int>);
  static_assert(std::output_iterator<any_iterator<int>, int>);
  static_assert(not std::semiregular<any_iterator<int, input>>);
  static_assert(any_iterator<int, forward>::kind == forward);
  static_assert(std::semiregular<any_iterator<int, forward>>);
  static_assert(not std::forward_iterator<any_iterator<int, input>>);
  static_assert(std::forward_iterator<any_iterator<int, forward>>);
  static_assert(not std::bidirectional_iterator<any_iterator<int, forward>>);
  static_assert(std::bidirectional_iterator<any_iterator<int, bidirectional>>);
  static_assert(not std::random_access_iterator<any_iterator<int, bidirectional>>);
  static_assert(std::random_access_iterator<any_iterator<int, random_access>>);
  static_assert(not std::contiguous_iterator<any_iterator<int, random_access>>);
  static_assert(std::contiguous_iterator<any_iterator<int, contiguous>>);
  // clang-format on
}
