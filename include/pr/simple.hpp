#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace pr {

struct _is_trait {};
struct _is_trait_of {};
struct _is_interface {};
struct _is_inplace {};
struct _is_pointer {};
struct _is_small {};
struct _is_table {};
struct _is_simple {};

template <typename T, typename Policy, typename... Ts>
inline constexpr bool _enable = false;

template <typename T>
concept trait = std::is_aggregate_v<T> and std::is_empty_v<T> and
                pr::_enable<T, pr::_is_trait>;

template <typename T>
concept _interface = pr::_enable<T, pr::_is_interface>;

template <typename T>
concept _inplace = pr::_enable<T, pr::_is_inplace>;

template <typename T>
concept _pointer = pr::_enable<T, pr::_is_pointer>;

template <typename T>
concept _small = pr::_enable<T, pr::_is_small>;

template <typename T>
concept _storage = pr::_inplace<T> or pr::_pointer<T> or pr::_small<T>;

template <typename T, typename Storage>
concept _storable_to =
    pr::_storage<Storage> and requires(std::in_place_type_t<T> tag, T func()) {
      { Storage::make_storage(tag, func) } -> std::same_as<Storage>;
    };

template <typename T>
concept _table = pr::_enable<T, pr::_is_table>;

template <typename T>
concept _simple = pr::_enable<T, pr::_is_simple>;

template <typename Trait, typename Simple>
concept trait_of = pr::trait<Trait> and pr::_simple<Simple> and
                   pr::_enable<Trait, pr::_is_trait_of, Simple>;

template <pr::trait Trait, pr::_simple Simple>
inline constexpr bool _enable<Trait, pr::_is_trait_of, Simple> =
    pr::_enable<Trait, pr::_is_trait_of, typename Simple::interface_type>;

template <typename Simple, typename Trait>
concept simplifies = pr::trait_of<Trait, Simple>;

template <typename T, typename Trait>
concept implements = not pr::_simple<std::remove_cvref_t<T>> and
                     pr::trait<Trait> and Trait::template enable<T>;

template <typename T, typename U>
concept _different_from =
    not std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename T, typename Wrapper, typename Interface>
concept concrete = pr::_different_from<T, Wrapper> and
                   pr::_interface<Interface> and implements<T, Interface>;

template <typename F, typename R, typename... Args>
concept _invocable_r = std::is_invocable_r_v<R, F, Args...>;

template <typename T>
concept _reference = std::is_reference_v<T>;

template <typename T>
concept _prvalue = not pr::_reference<T>;

template <typename T>
using _add_pointer_if_reference_t =
    std::conditional_t<pr::_reference<T>, std::add_pointer_t<T>, T>;

template <typename T, typename Storage>
concept _inplace_storable_to =
    pr::_storage<Storage> and not pr::_pointer<Storage> and
    sizeof(pr::_add_pointer_if_reference_t<T>) <= sizeof(Storage) and
    alignof(Storage) % alignof(pr::_add_pointer_if_reference_t<T>) == 0;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct inplace_storage {
  // NOLINTNEXTLINE(*-avoid-c-arrays)
  using buffer_type = std::byte[Size];

  alignas(Align) buffer_type m_buffer;

  template <pr::_inplace_storable_to<inplace_storage> T, pr::_invocable_r<T> F>
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  inplace_storage([[maybe_unused]] std::in_place_type_t<T> tag, F &&func) {
    if constexpr (pr::_reference<T>) {
      const T reference = std::invoke(std::forward<F>(func));
      ::new (m_buffer) std::add_pointer_t<T>(std::addressof(reference));
    } else {
      static_assert(pr::_prvalue<T>);
      ::new (m_buffer) T(std::invoke(std::forward<F>(func)));
    }
  }

  template <pr::_inplace_storable_to<inplace_storage> T, pr::_invocable_r<T> F>
  static auto make_storage(std::in_place_type_t<T> tag,
                           F &&func) -> inplace_storage {
    return inplace_storage{tag, std::forward<F>(func)};
  }
};

template <auto Size, auto Align>
inline constexpr bool
    _enable<pr::inplace_storage<Size, Align>, pr::_is_inplace> = true;

struct pointer_storage {
  using pointer_type = void *;

  pointer_type m_pointer;

  template <typename T, pr::_invocable_r<T> F>
  static constexpr auto
  make_storage([[maybe_unused]] std::in_place_type_t<T> tag,
               F &&func) noexcept(pr::_reference<T>) -> pointer_storage {
    using type = std::remove_cvref_t<T>;

    if constexpr (pr::_reference<T>) {
      const T reference = std::invoke(std::forward<F>(func));
      return pointer_storage{.m_pointer =
                                 const_cast<type *>(std::addressof(reference))};
    } else {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      T *const pointer = new T(std::invoke(std::forward<F>(func)));
      return pointer_storage{.m_pointer = const_cast<type *>(pointer)};
    }
  }

  template <typename T>
  static constexpr auto
  make_storage(std::unique_ptr<T> &&ptr) noexcept -> pointer_storage {
    return pointer_storage{.m_pointer = std::move(ptr).release()};
  }
};

template <>
inline constexpr bool _enable<pr::pointer_storage, pr::_is_pointer> = true;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct small_storage {
  using inplace_type = pr::inplace_storage<Size, Align>;
  using pointer_type = pr::pointer_storage;

  union {
    inplace_type m_inplace;
    pointer_type m_pointer;
  };

  template <typename T, pr::_invocable_r<T> F>
  static constexpr auto
  make_storage(std::in_place_type_t<T> tag,
               F &&func) noexcept(pr::_inplace_storable_to<T, inplace_type> or
                                  pr::_reference<T>) -> small_storage {
    if constexpr (pr::_inplace_storable_to<T, inplace_type> and
                  pr::_prvalue<T>) {
      if (not std::is_constant_evaluated()) {
        return small_storage{
            .m_inplace{inplace_type::make_storage(tag, std::forward<F>(func))}};
      }
    }

    return small_storage{
        .m_pointer = pointer_type::make_storage(tag, std::forward<F>(func))};
  }

  template <typename T>
  static constexpr auto
  make_storage(std::unique_ptr<T> &&ptr) noexcept -> small_storage {
    return small_storage{.m_pointer =
                             pointer_type::make_storage(std::move(ptr))};
  }
};

template <auto Size, auto Align>
inline constexpr bool _enable<pr::small_storage<Size, Align>, pr::_is_small> =
    true;

struct trait_base {};

template <typename T>
inline constexpr bool _enable<T, pr::_is_trait> =
    std::derived_from<T, pr::trait_base>;

template <pr::trait... Traits>
struct interface : pr::trait_base {
  template <typename T>
  static constexpr bool enable = (... and pr::implements<T, Traits>);
};

template <typename... Traits>
inline constexpr bool _enable<pr::interface<Traits...>, pr::_is_interface> =
    true;

struct _destroys : pr::trait_base {
  template <std::destructible T>
  static constexpr bool enable = true;

  template <pr::simplifies<_destroys> Simple>
  static constexpr void fn(Simple &object) noexcept;

  template <pr::implements<_destroys> T, typename Policy>
  static constexpr void fn(void *&ptr) noexcept {
    if constexpr (pr::_prvalue<T>) {
      if constexpr (std::is_same_v<Policy, pr::_is_inplace>) {
        static_cast<T *>(ptr)->~T();
      } else if (ptr != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete static_cast<T *>(std::exchange(ptr, nullptr));
      }
    }
  }
};

struct move_constructs : pr::trait_base {
  template <typename T>
  static constexpr bool enable = std::is_nothrow_move_constructible_v<T>;

  template <pr::simplifies<move_constructs> Simple>
  static constexpr auto fn(Simple &&object) noexcept -> Simple;

  template <pr::implements<move_constructs> T, typename Policy>
  static constexpr auto fn(void *&ptr) noexcept -> decltype(auto) {
    if constexpr (pr::_reference<T> or
                  std::is_same_v<Policy, pr::_is_inplace>) {
      return T(static_cast<T &&>(*static_cast<std::add_pointer_t<T>>(ptr)));
    } else {
      return std::unique_ptr<T>{static_cast<T *>(std::exchange(ptr, nullptr))};
    }
  }
};

template <pr::trait Trait, typename... Traits>
inline constexpr bool
    _enable<Trait, pr::_is_trait_of, pr::interface<Traits...>> =
        (std::same_as<Trait, Traits> or ... or std::same_as<Trait, _destroys>);

template <pr::trait Trait, pr::_simple Simple>
using _function_t = decltype(Trait::template fn<Simple>);

template <pr::trait Trait, pr::_simple Simple>
struct _vtable {
  pr::_function_t<Trait, Simple> *m_function;
};

template <typename... Traits, pr::_simple Simple>
struct _vtable<pr::interface<Traits...>, Simple>
    : pr::_vtable<Traits, Simple>..., pr::_vtable<pr::_destroys, Simple> {};

template <std::same_as<pr::_is_inplace> Policy>
auto _get_pointer(pr::_inplace auto &storage) noexcept -> void *& {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<void *&>(storage.m_buffer);
}

template <std::same_as<pr::_is_inplace> Policy>
constexpr auto _get_pointer(const pr::_inplace auto &storage) noexcept {
  return static_cast<const void *>(storage.m_buffer);
}

template <std::same_as<pr::_is_pointer> Policy>
constexpr auto _get_pointer(pr::_pointer auto &storage) noexcept -> void *& {
  return storage.m_pointer;
}

template <std::same_as<pr::_is_pointer> Policy>
constexpr auto _get_pointer(const pr::_pointer auto &storage) noexcept {
  return storage.m_pointer;
}

template <std::same_as<pr::_is_inplace> Policy>
constexpr auto _get_pointer(pr::_small auto &storage) noexcept -> void *& {
  return pr::_get_pointer<Policy>(storage.m_inplace);
}

template <std::same_as<pr::_is_inplace> Policy>
constexpr auto _get_pointer(const pr::_small auto &storage) noexcept {
  return pr::_get_pointer<Policy>(storage.m_inplace);
}

template <std::same_as<pr::_is_pointer> Policy>
constexpr auto _get_pointer(pr::_small auto &storage) noexcept -> void *& {
  return pr::_get_pointer<Policy>(storage.m_pointer);
}

template <std::same_as<pr::_is_pointer> Policy>
constexpr auto _get_pointer(const pr::_small auto &storage) noexcept {
  return pr::_get_pointer<Policy>(storage.m_pointer);
}

template <pr::_prvalue T, typename Policy>
constexpr auto _unwrap(pr::_storage auto &storage) noexcept -> T & {
  return *static_cast<T *>(pr::_get_pointer<Policy>(storage));
}

template <pr::_prvalue T, typename Policy>
constexpr auto _unwrap(const pr::_storage auto &storage) noexcept -> const T & {
  return *static_cast<const T *>(pr::_get_pointer<Policy>(storage));
}

template <pr::_prvalue T, typename Policy>
constexpr auto _unwrap(pr::_storage auto &&storage) noexcept -> T && {
  return std::move(*static_cast<T *>(pr::_get_pointer<Policy>(storage)));
}

template <pr::_prvalue T, typename Policy>
constexpr auto
_unwrap(const pr::_storage auto &&storage) noexcept -> const T && {
  return std::move(*static_cast<const T *>(pr::_get_pointer<Policy>(storage)));
}

template <pr::_reference T, typename Policy>
constexpr auto _unwrap(const pr::_storage auto &storage) noexcept -> T {
  return static_cast<T>(
      *static_cast<std::add_pointer_t<T>>(pr::_get_pointer<Policy>(storage)));
}

template <typename T, typename Policy, typename Param, typename Arg>
constexpr auto _forward_or_unwrap(Arg &&arg) noexcept -> decltype(auto) {
  if constexpr (std::is_same_v<Param &&, Arg &&>) {
    return std::forward<Arg>(arg);
  } else {
    static_assert(pr::_simple<std::remove_cvref_t<Arg>>);

    if constexpr (std::same_as<Param, void *&>) {
      // for _destroys and nothrow_move_constructs traits
      return pr::_get_pointer<Policy>(arg.m_storage);
    } else {
      return static_cast<Param>(
          pr::_unwrap<T, Policy>(std::forward<Arg>(arg).m_storage));
    }
  }
}

template <typename T, typename Policy, typename R, typename... Params,
          bool Noexcept, typename... Args>
constexpr auto _invoke(R (*func)(Params...) noexcept(Noexcept),
                       Args &&...args) noexcept(Noexcept) -> R {
  return func(
      pr::_forward_or_unwrap<T, Policy, Params>(std::forward<Args>(args))...);
}

template <pr::trait Trait, pr::_simple Simple, pr::implements<Trait> T,
          typename Policy, typename = pr::_function_t<Trait, Simple>>
struct _bind;

template <pr::trait Trait, pr::_simple Simple, pr::implements<Trait> T,
          typename Policy, typename R, typename... Args, bool Noexcept>
struct _bind<Trait, Simple, T, Policy, R(Args...) noexcept(Noexcept)> {
  static constexpr auto fn(Args &&...args) noexcept(Noexcept) -> R {
    if constexpr (requires { Trait::template fn<T, Policy>; }) {
      // for _destroys and nothrow_move_constructs traits
      return pr::_invoke<T, Policy>(Trait::template fn<T, Policy>,
                                    std::forward<Args>(args)...);
    } else {
      return pr::_invoke<T, Policy>(Trait::template fn<T>,
                                    std::forward<Args>(args)...);
    }
  }
};

template <pr::trait Trait, pr::_simple Simple, pr::implements<Trait> T,
          typename Policy>
inline constexpr auto _vtable_v = pr::_vtable<Trait, Simple>{
    .m_function = pr::_bind<Trait, Simple, T, Policy>::fn};

template <typename... Traits, pr::_simple Simple, typename T, typename Policy>
inline constexpr auto _vtable_v<pr::interface<Traits...>, Simple, T, Policy> =
    pr::_vtable<pr::interface<Traits...>, Simple>{
        pr::_vtable_v<Traits, Simple, T, Policy>...,
        pr::_vtable_v<pr::_destroys, Simple, T, Policy>};

struct _table_base {};

template <typename T>
inline constexpr bool _enable<T, pr::_is_table> =
    std::derived_from<T, pr::_table_base>;

struct inplace_table : pr::_table_base {
  template <pr::_simple Simple>
  struct type {
    using interface_type = typename Simple::interface_type;
    using vtable_type = pr::_vtable<interface_type, Simple>;

    vtable_type m_vtable;

    template <typename Policy, pr::implements<interface_type> T>
    static constexpr auto
    make_table([[maybe_unused]] std::in_place_type_t<T> tag) -> type {
      return type{.m_vtable = pr::_vtable_v<interface_type, Simple, T, Policy>};
    }

    template <pr::trait_of<Simple> Trait>
    constexpr auto operator[]([[maybe_unused]] Trait key) const noexcept
        -> pr::_function_t<Trait, Simple> * {
      return static_cast<const pr::_vtable<Trait, Simple> &>(m_vtable)
          .m_function;
    }
  };
};

struct pointer_table : pr::_table_base {
  template <pr::_simple Simple>
  struct type {
    using interface_type = typename Simple::interface_type;
    using vtable_type = pr::_vtable<interface_type, Simple>;

    const vtable_type *m_vtable;

    template <typename Policy, pr::implements<interface_type> T>
    static constexpr auto
    make_table([[maybe_unused]] std::in_place_type_t<T> tag) -> type {
      return type{.m_vtable = std::addressof(
                      pr::_vtable_v<interface_type, Simple, T, Policy>)};
    }

    template <pr::trait_of<Simple> Trait>
    constexpr auto operator[]([[maybe_unused]] Trait key) const noexcept
        -> pr::_function_t<Trait, Simple> * {
      return static_cast<const pr::_vtable<Trait, Simple> *>(m_vtable)
          ->m_function;
    }
  };
};

template <pr::_interface Interface,
          pr::_storage Storage = pr::small_storage<sizeof(void *)>,
          pr::_table Table = pr::pointer_table>
class simple {
  template <typename T, typename Policy, typename Param, typename Arg>
  friend constexpr auto
  pr::_forward_or_unwrap(Arg &&arg) noexcept -> decltype(auto);

  template <typename T>
  static consteval auto get_noexcept() noexcept -> bool {
    return pr::_inplace<storage_type> or pr::_reference<T> or
           pr::_inplace_storable_to<T, storage_type>;
  }

public:
  using interface_type = Interface;
  using storage_type = Storage;
  using table_type = Table::template type<simple>;

private:
  table_type m_table;
  storage_type m_storage;

public:
  // participates when storage_type is a specialization of inplace_storage
  template <pr::implements<interface_type> T, pr::_invocable_r<T> F>
    requires pr::_inplace<storage_type> and
                 pr::_inplace_storable_to<T, storage_type>
  simple(std::in_place_type_t<T> tag,
         F &&func) noexcept(std::is_nothrow_invocable_r_v<T, F>)
      : m_table{table_type::template make_table<pr::_is_inplace>(tag)},
        m_storage{storage_type::make_storage(tag, std::forward<F>(func))} {}

  // participates when storage_type supports pointer_storage
  template <pr::implements<interface_type> T, pr::_invocable_r<T> F>
    requires pr::_pointer<storage_type> or pr::_small<storage_type>
  constexpr simple(std::in_place_type_t<T> tag,
                   F &&func) noexcept(std::is_nothrow_invocable_r_v<T, F> and
                                      pr::_reference<T>)
      : m_table{table_type::template make_table<pr::_is_pointer>(tag)},
        m_storage{storage_type::make_storage(tag, std::forward<F>(func))} {}

  // participates when storage_type is a specialization of small_storage
  template <pr::implements<interface_type> T, pr::_invocable_r<T> F>
    requires pr::_small<storage_type> and
                 pr::_inplace_storable_to<T, storage_type> and pr::_prvalue<T>
  constexpr simple(std::in_place_type_t<T> tag,
                   F &&func) noexcept(std::is_nothrow_invocable_r_v<T, F>)
      : m_table{std::is_constant_evaluated()
                    ? table_type::template make_table<pr::_is_pointer>(tag)
                    : table_type::template make_table<pr::_is_inplace>(tag)},
        m_storage{storage_type::make_storage(tag, std::forward<F>(func))} {}

  template <pr::implements<interface_type> T, typename... Args>
    requires pr::_storable_to<T, storage_type> and
             std::constructible_from<T, Args &&...>
  constexpr simple(std::in_place_type_t<T> tag, Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args &&...> and
      simple::get_noexcept<T>())
      : simple{tag,
               [&]() noexcept(std::is_nothrow_constructible_v<T, Args &&...>)
                   -> T { return T(std::forward<Args>(args)...); }} {}

  template <pr::implements<interface_type> T>
    requires pr::_storable_to<T, storage_type> and
             std::constructible_from<T, T &&>
  constexpr simple(T &&object) noexcept(
      std::is_nothrow_constructible_v<T, T &&> and simple::get_noexcept<T>())
      : simple{std::in_place_type<T>, std::forward<T>(object)} {}

  // assumes ownership of concrete type
  template <pr::implements<interface_type> T>
    requires pr::_pointer<storage_type> or pr::_small<storage_type>
  constexpr simple(std::unique_ptr<T> &&ptr) noexcept
      : m_table{table_type::template make_table<pr::_is_pointer>(
            std::in_place_type<T>)},
        m_storage{storage_type::make_storage(std::move(ptr))} {}

  simple(const simple &) = delete;
  simple(simple &&) = delete;

  auto operator=(const simple &) -> simple & = delete;
  auto operator=(simple &&) -> simple & = delete;

  constexpr auto operator[](pr::trait_of<simple> auto trait) const noexcept {
    return m_table[trait];
  }

  constexpr ~simple() noexcept(pr::_inplace<storage_type>) {
    m_table[pr::_destroys{}](*this);
  }
};

template <typename Interface, typename Storage, typename Table>
inline constexpr bool
    _enable<pr::simple<Interface, Storage, Table>, pr::_is_simple> = true;

} // namespace pr
