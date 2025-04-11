#pragma once

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

namespace pr {

template <typename T, template <typename...> typename L>
inline constexpr bool _is_type_specialization_of_v = false;

template <typename... Ts, template <typename...> typename L>
inline constexpr bool _is_type_specialization_of_v<L<Ts...>, L> = true;

template <typename T, template <auto...> typename L>
inline constexpr bool _is_value_specialization_of_v = false;

template <auto... Vs, template <auto...> typename L>
inline constexpr bool _is_value_specialization_of_v<L<Vs...>, L> = true;

template <typename T, template <typename...> typename L>
concept _type_specialization_of = pr::_is_type_specialization_of_v<T, L>;

template <typename T, template <auto...> typename L>
concept _value_specialization_of = pr::_is_value_specialization_of_v<T, L>;

struct trait_base {};

template <typename T>
concept trait = std::is_aggregate_v<T> and std::is_empty_v<T> and
                std::is_base_of_v<pr::trait_base, T>;

template <pr::trait... Traits>
struct interface;

template <typename T>
concept _interface = pr::_type_specialization_of<T, pr::interface>;

struct _table_base {};

template <typename T>
concept _table = std::is_base_of_v<pr::_table_base, T>;

struct inplace_table;

struct pointer_table;

template <typename T>
inline constexpr bool _enable_storage = false;

template <typename T>
concept _storage = pr::_enable_storage<T>;

template <typename T>
struct _wrapper {
  T m_value;
};

template <typename T, typename Storage>
concept _small = pr::_storage<Storage> and
                 sizeof(pr::_wrapper<T>) <= Storage::size_bytes() and
                 Storage::align_bytes() % alignof(pr::_wrapper<T>) == 0;

template <typename F, typename R, typename... Args>
concept _invocable_r = std::is_invocable_r_v<R, F, Args...>;

template <typename T>
concept _reference = std::is_reference_v<T>;

template <typename T>
concept _non_reference = not pr::_reference<T>;

enum class _storage_index : bool {
  _is_inplace,
  _is_pointer,
};

using enum pr::_storage_index;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct inplace_storage {
  using buffer_type = std::array<std::byte, Size>;

  static consteval auto size_bytes() noexcept -> std::size_t { return Size; }
  static consteval auto align_bytes() noexcept -> std::size_t { return Align; }

  alignas(Align) buffer_type m_buffer;

  template <pr::_small<inplace_storage> T, pr::_invocable_r<T> F>
  inplace_storage(std::in_place_type_t<T> _, F &&f) {
    using inplace_type =
        std::conditional_t<pr::_reference<T>, pr::_wrapper<T>, T>;

    ::new (static_cast<void *>(&m_buffer))
        inplace_type(std::invoke(std::forward<F>(f)));
  }

  template <pr::_small<inplace_storage> T, pr::_invocable_r<T> F>
  static auto make_storage(std::in_place_type_t<T> type,
                           F &&f) -> inplace_storage {
    return {type, std::forward<F>(f)};
  }
};

template <pr::_value_specialization_of<pr::inplace_storage> T>
inline constexpr bool _enable_storage<T> = true;

struct pointer_storage {
  using pointer_type = void *;

  pointer_type m_pointer;

  template <pr::_reference T, pr::_invocable_r<T> F>
  static constexpr auto make_storage(std::in_place_type_t<T> _,
                                     F &&f) -> pointer_storage {
    using type = std::remove_cvref_t<T>;
    const T reference = std::invoke(std::forward<F>(f));
    return {.m_pointer{const_cast<type *>(std::addressof(reference))}};
  }

  template <pr::_non_reference T, pr::_invocable_r<T> F>
  static constexpr auto make_storage(std::in_place_type_t<T> _,
                                     F &&f) -> pointer_storage {
    using type = std::remove_cv_t<T>;
    T *const pointer = new T(std::invoke(std::forward<F>(f)));
    return {.m_pointer{const_cast<type *>(pointer)}};
  }
};

template <std::same_as<pointer_storage> T>
inline constexpr bool _enable_storage<T> = true;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct small_storage {
  using inplace_type = pr::inplace_storage<Size, Align>;
  using pointer_type = pr::pointer_storage;

  union {
    inplace_type m_inplace;
    pointer_type m_pointer;
  };

  static consteval auto size_bytes() noexcept -> std::size_t { return Size; }
  static consteval auto align_bytes() noexcept -> std::size_t { return Align; }

  template <typename T, pr::_invocable_r<T> F>
  static constexpr auto make_storage(std::in_place_type_t<T> type,
                                     F &&f) -> small_storage {
    if constexpr (pr::_small<T, small_storage> and pr::_non_reference<T>) {
      if (not std::is_constant_evaluated()) {
        return {
            .m_inplace{inplace_type::make_storage(type, std::forward<F>(f))}};
      }
    }

    return {.m_pointer{pointer_type::make_storage(type, std::forward<F>(f))}};
  }
};

template <pr::_value_specialization_of<small_storage> T>
inline constexpr bool _enable_storage<T> = true;

template <typename T>
concept _inplace_storage =
    _storage<T> and pr::_value_specialization_of<T, inplace_storage>;

template <typename T>
concept _pointer_storage = _storage<T> and std::same_as<T, pointer_storage>;

template <typename T>
concept _small_storage =
    _storage<T> and pr::_value_specialization_of<T, small_storage>;

template <pr::_interface Interface,
          pr::_storage Storage = pr::small_storage<sizeof(void *)>,
          pr::_table Table = pr::pointer_table>
class impl;

template <typename T>
concept _impl = pr::_type_specialization_of<T, pr::impl>;

template <typename T, typename Trait>
concept declares = pr::_impl<T> and pr::trait<Trait>;

template <typename T, typename Trait>
concept implements = not pr::_impl<std::remove_cvref_t<T>> and
                     pr::trait<Trait> and Trait::template enable<T>;

template <pr::trait... Traits>
struct interface : pr::trait_base {
  template <typename T>
  static constexpr bool enable = (... and pr::implements<T, Traits>);
};

template <typename T, typename U>
concept _different_from =
    not std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename T, typename Wrapper, typename Interface>
concept concrete = _different_from<T, Wrapper> and _interface<Interface> and
                   implements<T, Interface>;

struct _destroys : pr::trait_base {
  template <std::destructible T>
  static constexpr bool enable = true;

  template <pr::declares<_destroys> Impl>
  static constexpr void fn(Impl &object) noexcept;

  template <pr::implements<_destroys> T, pr::_storage_index Index>
  static constexpr void fn(T &object) noexcept {
    if constexpr (pr::_non_reference<T>) {
      if constexpr (Index == pr::_is_inplace) {
        object.~T();
      } else {
        delete std::addressof(object);
      }
    }
  }
};

template <typename T, typename Storage>
concept _storable = requires(std::in_place_type_t<T> type, T f()) {
  { Storage::make_storage(type, f) } -> std::same_as<Storage>;
};

template <pr::trait Trait, typename T>
using _function_t = decltype(Trait::template fn<T>);

template <pr::trait Trait, pr::_impl Impl>
struct _vtable {
  pr::_function_t<Trait, Impl> *m_function;
};

template <typename... Traits, pr::_impl Impl>
struct _vtable<pr::interface<Traits...>, Impl>
    : pr::_vtable<Traits, Impl>..., pr::_vtable<pr::_destroys, Impl> {};

template <typename Trait, typename Impl>
concept _trait_of =
    pr::trait<Trait> and pr::_impl<Impl> and
    std::is_base_of_v<pr::_vtable<Trait, Impl>,
                      pr::_vtable<typename Impl::interface_type, Impl>>;

template <pr::_interface Interface, pr::_storage Storage, pr::_table Table>
class impl {
  template <typename T, pr::_storage_index Index, typename Param, typename Arg>
  friend constexpr auto
  _forward_or_unwrap(Arg &&arg) noexcept -> decltype(auto);

public:
  using interface_type = Interface;
  using storage_type = Storage;
  using table_type = Table::template type<impl>;

private:
  table_type m_table;
  storage_type m_storage;

public:
  // participates when storage_type is a specialization of inplace_storage
  template <pr::implements<interface_type> T, pr::_invocable_r<T> F>
    requires pr::_inplace_storage<storage_type> and pr::_small<T, storage_type>
  impl(std::in_place_type_t<T> type,
       F &&f) noexcept(std::is_nothrow_invocable_r_v<T, F>)
      : m_table{table_type::template make_table<pr::_is_inplace>(type)},
        m_storage{storage_type::make_storage(type, std::forward<F>(f))} {}

  // participates when storage_type supports pointer_storage
  template <pr::implements<interface_type> T, pr::_invocable_r<T> F>
    requires pr::_pointer_storage<storage_type> or
                 pr::_small_storage<storage_type>
  constexpr impl(std::in_place_type_t<T> type, F &&f)
      : m_table{table_type::template make_table<pr::_is_pointer>(type)},
        m_storage{storage_type::make_storage(type, std::forward<F>(f))} {}

  // participates when storage_type is a specialization of small_storage
  template <pr::implements<interface_type> T, pr::_invocable_r<T> F>
    requires pr::_small_storage<storage_type> and
                 pr::_small<T, storage_type> and pr::_non_reference<T>
  constexpr impl(std::in_place_type_t<T> type, F &&f)
      : m_table{std::is_constant_evaluated()
                    ? table_type::template make_table<pr::_is_pointer>(type)
                    : table_type::template make_table<pr::_is_inplace>(type)},
        m_storage{storage_type::make_storage(type, std::forward<F>(f))} {}

  template <pr::implements<interface_type> T, typename... Args>
    requires pr::_storable<T, storage_type> and
             std::constructible_from<T, Args &&...>
  constexpr impl(std::in_place_type_t<T> type, Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args &&...> and
      pr::_inplace_storage<storage_type>)
      : impl{type,
             [&]() noexcept(std::is_nothrow_constructible_v<T, Args &&...>)
                 -> T { return T(std::forward<Args>(args)...); }} {}

  template <pr::implements<interface_type> T>
    requires pr::_storable<T, storage_type> and std::constructible_from<T, T &&>
  constexpr impl(T &&object) noexcept(
      std::is_nothrow_constructible_v<T, T &&> and
      pr::_inplace_storage<storage_type>)
      : impl{std::in_place_type<T>, std::forward<T>(object)} {}

  impl(const impl &) = delete;
  impl(impl &&) = delete;

  auto operator=(const impl &) -> impl & = delete;
  auto operator=(impl &&) -> impl & = delete;

  constexpr auto operator[](pr::_trait_of<impl> auto trait) const noexcept {
    return m_table[trait];
  }

  constexpr ~impl() noexcept(pr::_inplace_storage<storage_type>) {
    m_table[pr::_destroys{}](*this);
  }
};

template <pr::_storage_index Index, std::size_t Size, std::size_t Align>
constexpr auto
_get_pointer(pr::inplace_storage<Size, Align> &storage) noexcept {
  static_assert(Index == pr::_is_inplace);
  return static_cast<void *>(&storage.m_buffer);
}

template <pr::_storage_index Index, std::size_t Size, std::size_t Align>
constexpr auto
_get_pointer(const pr::inplace_storage<Size, Align> &storage) noexcept {
  static_assert(Index == pr::_is_inplace);
  return static_cast<const void *>(&storage.m_buffer);
}

template <pr::_storage_index Index>
constexpr auto _get_pointer(const pr::pointer_storage &storage) noexcept {
  static_assert(Index == pr::_is_pointer);
  return storage.m_pointer;
}

template <pr::_storage_index Index, std::size_t Size, std::size_t Align>
constexpr auto _get_pointer(pr::small_storage<Size, Align> &storage) noexcept {
  if constexpr (Index == pr::_is_inplace) {
    return pr::_get_pointer<Index>(storage.m_inplace);
  } else {
    return pr::_get_pointer<Index>(storage.m_pointer);
  }
}

template <pr::_storage_index Index, std::size_t Size, std::size_t Align>
constexpr auto
_get_pointer(const pr::small_storage<Size, Align> &storage) noexcept {
  if constexpr (Index == pr::_is_inplace) {
    return pr::_get_pointer<Index>(storage.m_inplace);
  } else {
    return pr::_get_pointer<Index>(storage.m_pointer);
  }
}

template <pr::_non_reference T, pr::_storage_index Index>
constexpr auto _unwrap(pr::_storage auto &storage) noexcept -> T & {
  return *static_cast<T *>(pr::_get_pointer<Index>(storage));
}

template <pr::_non_reference T, pr::_storage_index Index>
constexpr auto _unwrap(const pr::_storage auto &storage) noexcept -> const T & {
  return *static_cast<const T *>(pr::_get_pointer<Index>(storage));
}

template <pr::_non_reference T, pr::_storage_index Index>
constexpr auto _unwrap(pr::_storage auto &&storage) noexcept -> T && {
  return std::move(*static_cast<T *>(pr::_get_pointer<Index>(storage)));
}

template <pr::_non_reference T, pr::_storage_index Index>
constexpr auto
_unwrap(const pr::_storage auto &&storage) noexcept -> const T && {
  return std::move(*static_cast<const T *>(pr::_get_pointer<Index>(storage)));
}

template <pr::_reference T, pr::_storage_index Index>
constexpr auto _unwrap(const pr::_storage auto &storage) noexcept -> T {
  if constexpr (Index == pr::_is_inplace) {
    return static_cast<T>(
        static_cast<const pr::_wrapper<T> *>(pr::_get_pointer<Index>(storage))
            ->m_value);
  } else {
    return static_cast<T>(
        *static_cast<std::add_pointer_t<T>>(pr::_get_pointer<Index>(storage)));
  }
}

template <typename T, pr::_storage_index Index, typename Param, typename Arg>
constexpr auto _forward_or_unwrap(Arg &&arg) noexcept -> decltype(auto) {
  if constexpr (std::is_same_v<Param &&, Arg &&>) {
    return std::forward<Arg>(arg);
  } else {
    static_assert(pr::_impl<std::remove_cvref_t<Arg>>);
    return static_cast<Param>(
        pr::_unwrap<T, Index>(std::forward<Arg>(arg).m_storage));
  }
}

template <typename T, pr::_storage_index Index, typename R, typename... Params,
          bool Noexcept, typename... Args>
constexpr auto _invoke(R (*fn)(Params...) noexcept(Noexcept),
                       Args &&...args) noexcept(Noexcept) -> R {
  return fn(
      pr::_forward_or_unwrap<T, Index, Params>(std::forward<Args>(args))...);
}

template <pr::trait Trait, pr::_impl Impl, pr::implements<Trait> T,
          pr::_storage_index Index, typename = pr::_function_t<Trait, Impl>>
struct _bind;

template <pr::trait Trait, pr::_impl Impl, pr::implements<Trait> T,
          pr::_storage_index Index, typename R, typename... Args, bool Noexcept>
struct _bind<Trait, Impl, T, Index, R(Args...) noexcept(Noexcept)> {
  static constexpr auto fn(Args &&...args) noexcept(Noexcept) -> R {
    if constexpr (std::is_same_v<Trait, pr::_destroys>) {
      return pr::_invoke<T, Index>(pr::_destroys::fn<T, Index>,
                                   std::forward<Args>(args)...);
    } else {
      return pr::_invoke<T, Index>(Trait::template fn<T>,
                                   std::forward<Args>(args)...);
    }
  }
};

template <pr::trait Trait, pr::_impl Impl, pr::implements<Trait> T,
          pr::_storage_index Index>
inline constexpr pr::_vtable<Trait, Impl> _vtable_v{
    .m_function{pr::_bind<Trait, Impl, T, Index>::fn}};

template <typename... Traits, pr::_impl Impl, typename T,
          pr::_storage_index Index>
inline constexpr pr::_vtable<pr::interface<Traits...>, Impl>
    _vtable_v<pr::interface<Traits...>, Impl, T, Index>{
        pr::_vtable_v<Traits, Impl, T, Index>...,
        pr::_vtable_v<pr::_destroys, Impl, T, Index>};

struct inplace_table : pr::_table_base {
  template <pr::_impl Impl>
  struct type {
    using interface_type = typename Impl::interface_type;
    using vtable_type = pr::_vtable<interface_type, Impl>;

    vtable_type m_vtable;

    template <pr::_storage_index Index, pr::implements<interface_type> T>
    static consteval auto make_table(std::in_place_type_t<T> _) -> type {
      return {.m_vtable{pr::_vtable_v<interface_type, Impl, T, Index>}};
    }

    template <pr::_trait_of<Impl> Trait>
    constexpr auto
    operator[](Trait _) const noexcept -> pr::_function_t<Trait, Impl> * {
      return static_cast<const pr::_vtable<Trait, Impl> &>(m_vtable).m_function;
    }
  };
};

struct pointer_table : pr::_table_base {
  template <pr::_impl Impl>
  struct type {
    using interface_type = typename Impl::interface_type;
    using vtable_type = pr::_vtable<interface_type, Impl>;

    const vtable_type *m_vtable;

    template <pr::_storage_index Index, pr::implements<interface_type> T>
    static consteval auto make_table(std::in_place_type_t<T> _) -> type {
      return {.m_vtable{
          std::addressof(pr::_vtable_v<interface_type, Impl, T, Index>)}};
    }

    template <pr::_trait_of<Impl> Trait>
    constexpr auto
    operator[](Trait _) const noexcept -> pr::_function_t<Trait, Impl> * {
      return static_cast<const pr::_vtable<Trait, Impl> *>(m_vtable)
          ->m_function;
    }
  };
};

} // namespace pr
