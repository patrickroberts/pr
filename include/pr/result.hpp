#pragma once

#include <expected>
#include <optional>
#include <type_traits>

#define PR_MAYBE(...)                                                          \
  ({                                                                           \
    auto &&_result = (__VA_ARGS__);                                            \
    using _type = decltype(_result);                                           \
    if (not _result) {                                                         \
      return ::pr::error(static_cast<_type>(_result));                         \
    }                                                                          \
    *static_cast<_type>(_result);                                              \
  })

namespace pr {

template <class T, template <class...> class C>
inline constexpr bool is_specialization_of_v = false;

template <class... Args, template <class...> class C>
inline constexpr bool is_specialization_of_v<C<Args...>, C> = true;

namespace is {

template <class T, template <class...> class C>
concept specialization_of = is_specialization_of_v<T, C>;

template <class T, template <class...> class C>
concept cvref_specialization_of = specialization_of<std::remove_cvref_t<T>, C>;

template <class T>
concept pointer = std::is_pointer_v<T>;

} // namespace is

template <is::cvref_specialization_of<std::expected> T>
using unexpected_type_t = std::remove_cvref_t<T>::unexpected_type;

template <is::cvref_specialization_of<std::expected> T>
constexpr auto error(T &&expected) -> unexpected_type_t<T> {
  return unexpected_type_t<T>{std::forward<T>(expected).error()};
}

template <is::specialization_of<std::optional> T>
constexpr auto error([[maybe_unused]] const T &optional) -> std::nullopt_t {
  return std::nullopt;
}

template <is::pointer T>
constexpr auto error([[maybe_unused]] T pointer) -> std::nullptr_t {
  return nullptr;
}

} // namespace pr
