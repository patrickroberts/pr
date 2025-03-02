#pragma once

#include <utility>

namespace pr {
namespace detail {

template <class T>
concept storable_ = std::same_as<T, std::remove_cvref_t<T>>;

template <storable_ T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline thread_local constinit T *context_ = nullptr;

template <class T>
concept makeable_ =
    storable_<std::remove_reference_t<T>> and not std::is_rvalue_reference_v<T>;

template <makeable_ T>
class provider_ {
  using value_type = std::remove_reference_t<T>;

  // `mutable` prevents UB when `make_context` initializes a `const auto`
  [[no_unique_address]] mutable T inner_;
  value_type *outer_;

public:
  template <class... ArgsT>
    requires std::constructible_from<T, ArgsT...>
  explicit provider_(ArgsT &&...args) noexcept(
      std::is_nothrow_constructible_v<T, ArgsT...>)
      : inner_(std::forward<ArgsT>(args)...),
        outer_(std::exchange(context_<value_type>, std::addressof(inner_))) {}

  provider_(const provider_ &) = delete;
  provider_(provider_ &&) = delete;

  ~provider_() noexcept { context_<value_type> = outer_; }
};

template <class T>
concept gettable_ = storable_<std::remove_const_t<T>>;

} // namespace detail

/**
 * Given `value_type` as `std::remove_reference_t<T>` and the instantiation
 * `static thread_local value_type *context_<value_type> = nullptr;`,
 * calling this function constructs and returns an object whose constructor
 * value-initializes a private, exposition-only member
 * `[[no_unique_address]] T inner_;` as if by
 * `inner_(std::forward<ArgsT>(args)...)`, and another private,
 * exposition-only member `value_type *outer_;` as if by
 * `outer_(std::exchange(context_<value_type>, std::addressof(inner_)))`.
 * Upon destruction, the returned object restores `context_<value_type>` to the
 * value of `outer_`. The copy and move constructors of the return type are
 * deleted. `T` must be a cv-unqualified non-reference or lvalue-reference
 * type, or the instantiation is ill-formed, which can result in substitution
 * failure when the call appears in the immediate context of a template
 * instantiation.
 */
template <detail::makeable_ T, class... ArgsT>
  requires std::constructible_from<T, ArgsT...>
[[nodiscard]] auto make_context(ArgsT &&...args) noexcept(
    std::is_nothrow_constructible_v<T, ArgsT...>) -> detail::provider_<T> {
  return detail::provider_<T>(std::forward<ArgsT>(args)...);
}

/**
 * Given `value_type` as `std::remove_const_t<T>` and the instantiation
 * `static thread_local value_type *context_<value_type> = nullptr;`, calling
 * this function returns `context_<value_type>`, whose value has been
 * initialized by thread-local calls to `pr::make_context<value_type>(...)` or
 * `pr::make_context<value_type &>(...)`, or `nullptr` otherwise. `T` must be
 * an optionally const-qualified non-reference type, or the instantiation is
 * ill-formed, which can result in substitution failure when the call appears in
 * the immediate context of a template instantiation.
 */
template <detail::gettable_ T>
[[nodiscard]] auto get_context() noexcept -> T * {
  using value_type = std::remove_const_t<T>;
  return detail::context_<value_type>;
}

} // namespace pr
