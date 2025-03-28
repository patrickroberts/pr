#pragma once

#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <variant>

namespace pr {
namespace detail {

template <class T>
inline constexpr bool is_impl_ = false;

template <class T, class U>
concept different_from_ =
    not std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <class Fn, class R, class... Args>
concept invocable_r = std::invocable<Fn, Args...> and
                      std::convertible_to<std::invoke_result_t<Fn, Args...>, R>;

// provides sizeof and alignof for references
template <class T>
struct raw_ {
  T value;
};

template <class T, std::size_t Size, std::size_t Align>
concept small_ = sizeof(raw_<T>) <= Size and alignof(raw_<T>) <= Align;

template <class T>
struct get_fn_;

template <class T>
inline constexpr get_fn_<T> get_{};

template <class T>
  requires std::is_object_v<T>
struct get_fn_<T> {
  template <class Storage>
  static constexpr auto operator()(Storage &storage) noexcept -> T & {
    return static_cast<raw_<T> *>(storage.get())->value;
  }

  template <class Storage>
  static constexpr auto
  operator()(const Storage &storage) noexcept -> const T & {
    return static_cast<const raw_<T> *>(storage.get())->value;
  }

  template <class Storage>
  static constexpr auto operator()(Storage &&storage) noexcept -> T && {
    return std::move(*static_cast<raw_<T> *>(storage.get())).value;
  }

  template <class Storage>
  static constexpr auto
  operator()(const Storage &&storage) noexcept -> const T && {
    return std::move(*static_cast<const raw_<T> *>(storage.get())).value;
  }
};

template <class T>
  requires std::is_reference_v<T>
struct get_fn_<T> {
  template <class Storage>
  static constexpr auto operator()(const Storage &storage) noexcept -> T {
    return static_cast<T>(static_cast<const raw_<T> *>(storage.get())->value);
  }
};

template <class T, class... Args>
  requires std::constructible_from<T, Args &&...>
class constructor_ {
  std::tuple<Args &&...> args;

  static constexpr auto make(Args &&...args) -> T {
    return T(std::forward<Args>(args)...);
  }

public:
  constexpr constructor_(std::in_place_type_t<T> type, Args &&...args) noexcept
      : args(std::forward<Args>(args)...) {}

  constexpr auto operator()() const && -> T {
    return std::apply(make, std::move(args));
  }
};

} // namespace detail

struct trait_base {};

template <class T>
inline constexpr bool enable_trait = std::derived_from<T, trait_base>;

template <class T>
concept trait =
    std::is_aggregate_v<T> and std::is_empty_v<T> and enable_trait<T>;

template <class T, class Trait>
concept declares = detail::is_impl_<std::remove_cvref_t<T>> and trait<Trait>;

template <class T, class Trait>
concept implements = not detail::is_impl_<std::remove_cvref_t<T>> and
                     trait<Trait> and Trait::template enable<T>;

template <class T, class Abstract, class Interface>
concept concrete =
    detail::different_from_<T, Abstract> and implements<T, Interface>;

template <trait... Traits>
struct interface : trait_base {
  template <class T>
    requires(... and implements<T, Traits>)
  static constexpr bool enable = true;
};

namespace detail {

template <trait Trait, class T>
using fn_ = decltype(Trait::template fn<T>);

template <trait Trait, class Impl>
struct table_ {
  fn_<Trait, Impl> *method;
};

template <class... Traits, class Impl>
struct table_<interface<Traits...>, Impl> : table_<Traits, Impl>... {};

template <class T, class Param, class Arg>
constexpr auto unwrap_(Arg &&arg) -> decltype(auto) {
  if constexpr (std::same_as<Param &&, Arg &&>) {
    return std::forward<Arg>(arg);
  } else {
    static_assert(is_impl_<std::remove_cvref_t<Arg>>);
    return static_cast<Param>(get_<T>(std::forward<Arg>(arg).storage));
  }
}

template <class T, class R, class... Params, class... Args>
constexpr auto invoke_unwrap_(R (*fn)(Params...), Args &&...args) -> R {
  return std::invoke(fn,
                     detail::unwrap_<T, Params>(std::forward<Args>(args))...);
}

template <trait Trait, class Impl, implements<Trait> T,
          class = fn_<Trait, Impl>>
struct method_;

template <trait Trait, class Impl, implements<Trait> T, class R, class... Args,
          bool Noexcept>
struct method_<Trait, Impl, T, R(Args...) noexcept(Noexcept)> {
  static constexpr auto fn(Args... args) noexcept(Noexcept) -> R {
    return detail::invoke_unwrap_<T>(Trait::template fn<T>,
                                     std::forward<Args>(args)...);
  }
};

template <trait Trait, class Impl, class T>
inline constexpr table_<Trait, Impl> methods{method_<Trait, Impl, T>::fn};

// optimization to bypass invoke_unwrap<T> if signatures are compatible
template <trait Trait, class Impl, class T>
  requires std::convertible_to<fn_<Trait, T>, fn_<Trait, Impl>>
inline constexpr table_<Trait, Impl> methods<Trait, Impl, T>{
    Trait::template fn<T>};

template <class... Traits, class Impl, class T>
inline constexpr table_<interface<Traits...>, Impl>
    methods<interface<Traits...>, Impl, T>{methods<Traits, Impl, T>...};

template <class Storage>
struct destroys_ : trait_base {
  template <std::destructible T>
  static constexpr bool enable = true;

  template <declares<destroys_> T>
  static constexpr void fn(Storage &storage);

  template <implements<destroys_> T>
  static constexpr void fn(Storage &storage) {
    storage.template reset<T>();
  }
};

struct get_visitor_ {
  static constexpr auto operator()(auto &alternative) {
    return alternative.get();
  }
};

template <class T>
struct reset_visitor_ {
  static constexpr auto operator()(auto &alternative) {
    alternative.template reset<T>();
  }
};

inline constexpr get_visitor_ getter_{};

template <class T>
inline constexpr reset_visitor_<T> resetter_{};

template <class Trait, class Interface, class Impl>
concept trait_of_ =
    trait<Trait> and std::derived_from<detail::table_<Interface, Impl>,
                                       detail::table_<Trait, Impl>>;

} // namespace detail

// allows inplace storage if sizeof <= Size and alignof <= Align, otherwise
// allocates storage with operator new
template <std::size_t Size, std::size_t Align = alignof(void *)>
class small_storage;

// only allows inplace storage if sizeof <= Size and alignof <= Align
template <std::size_t Size, std::size_t Align = alignof(void *)>
class inplace_storage {
  template <class T>
  friend struct detail::get_fn_;

  friend struct detail::get_visitor_;

  template <class T>
  friend struct detail::reset_visitor_;

  friend struct detail::destroys_<inplace_storage>;

  friend class pr::small_storage<Size, Align>;

  template <trait Interface, class Storage, class Table>
  friend class impl;

  struct storage_type {
    alignas(Align) std::array<std::byte, Size> data;
  };

  storage_type storage;

  [[nodiscard]] constexpr auto get() noexcept -> void * {
    return std::addressof(storage);
  }

  [[nodiscard]] constexpr auto get() const noexcept -> const void * {
    return std::addressof(storage);
  }

  template <class T>
  constexpr void reset() {
    using raw_type = detail::raw_<T>;
    static_cast<raw_type *>(get())->~raw_type();
  }

public:
  template <detail::small_<Size, Align> T, detail::invocable_r<T> Fn>
  inplace_storage(std::in_place_type_t<T> type,
                  Fn &&fn) noexcept(std::is_nothrow_invocable_r_v<T, Fn>) {
    ::new (get()) detail::raw_<T>{std::invoke(std::forward<Fn>(fn))};
  }

  template <detail::small_<Size, Align> T, class... Args>
    requires std::constructible_from<T, Args &&...>
  inplace_storage(std::in_place_type_t<T> type, Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args &&...>)
      : inplace_storage(
            type, detail::constructor_{type, std::forward<Args>(args)...}) {}
};

// always allocates storage with operator new
class pointer_storage {
  template <class T>
  friend struct detail::get_fn_;

  friend struct detail::get_visitor_;

  template <class T>
  friend struct detail::reset_visitor_;

  friend struct detail::destroys_<pointer_storage>;

  template <std::size_t Size, std::size_t Align>
  friend class small_storage;

  template <trait Interface, class Storage, class Table>
  friend class impl;

  using pointer_type = void *;

  pointer_type storage;

  [[nodiscard]] constexpr auto get() noexcept -> void * { return storage; }

  [[nodiscard]] constexpr auto get() const noexcept -> const void * {
    return storage;
  }

  template <class T>
  constexpr void reset() {
    using raw_type = detail::raw_<T>;
    delete static_cast<raw_type *>(get());
  }

public:
  constexpr pointer_storage() noexcept = default;

  template <class T, detail::invocable_r<T> Fn>
  constexpr pointer_storage(std::in_place_type_t<T> type, Fn &&fn)
      : storage(new detail::raw_<T>{std::invoke(std::forward<Fn>(fn))}) {}

  template <class T, class... Args>
    requires std::constructible_from<T, Args &&...>
  constexpr pointer_storage(std::in_place_type_t<T> type, Args &&...args)
      : pointer_storage(
            type, detail::constructor_{type, std::forward<Args>(args)...}) {}
};

template <std::size_t Size, std::size_t Align>
class small_storage {
  template <class T>
  friend struct detail::get_fn_;

  friend struct detail::destroys_<small_storage>;

  template <trait Interface, class Storage, class Table>
  friend class impl;

  using inplace_type = inplace_storage<Size, Align>;
  using pointer_type = pointer_storage;

  std::variant<pointer_type, inplace_type> storage;

  [[nodiscard]] constexpr auto get() noexcept -> void * {
    return std::visit(detail::getter_, storage);
  }

  [[nodiscard]] constexpr auto get() const noexcept -> const void * {
    return std::visit(detail::getter_, storage);
  }

  template <class T>
  constexpr void reset() {
    std::visit(detail::resetter_<T>, storage);
  }

public:
  template <class T, detail::invocable_r<T> Fn>
  constexpr small_storage(std::in_place_type_t<T> type, Fn &&fn) noexcept(
      detail::small_<T, Size, Align> and std::is_nothrow_invocable_r_v<T, Fn>) {
    if constexpr (detail::small_<T, Size, Align>) {
      if (not std::is_constant_evaluated()) {
        std::construct_at(std::addressof(storage),
                          std::in_place_type<inplace_type>, std::move(type),
                          std::forward<Fn>(fn));
        return;
      }
    }

    std::construct_at(std::addressof(storage), std::in_place_type<pointer_type>,
                      std::move(type), std::forward<Fn>(fn));
  }

  template <class T, class... Args>
    requires std::constructible_from<T, Args &&...>
  constexpr small_storage(
      std::in_place_type_t<T> type,
      Args &&...args) noexcept(detail::small_<T, Size, Align> and
                               std::is_nothrow_constructible_v<T, Args &&...>)
      : small_storage(type,
                      detail::constructor_{type, std::forward<Args>(args)...}) {
  }
};

using default_storage = small_storage<sizeof(void *)>;

// stores vtable as part of class layout
struct inplace_table {
  template <trait Interface, class Impl>
  struct type {
    template <detail::trait_of_<Interface, Impl> Trait>
    using method_type = detail::table_<Trait, Impl>;

    method_type<Interface> methods;

    template <implements<Interface> T>
    static consteval auto make() -> type {
      return {detail::methods<Interface, Impl, T>};
    }

    template <detail::trait_of_<Interface, Impl> Trait, class... Args>
    constexpr auto operator()(Trait trait, Args &&...args) const
        -> detail::fn_<Trait, Impl> * {
      return static_cast<const method_type<Trait> &>(methods).method;
    }
  };
};

// stores pointer to vtable
struct pointer_table {
  template <trait Interface, class Impl>
  struct type {
    template <trait Trait>
    using method_type = detail::table_<Trait, Impl>;

    const method_type<Interface> *methods;

    template <implements<Interface> T>
    static consteval auto make() -> type {
      return {std::addressof(detail::methods<Interface, Impl, T>)};
    }

    template <detail::trait_of_<Interface, Impl> Trait>
    constexpr auto operator[](Trait trait) const -> detail::fn_<Trait, Impl> * {
      return static_cast<const method_type<Trait> &>(*methods).method;
    }
  };
};

using default_table = pointer_table;

template <trait Interface, class Storage = default_storage,
          class Table = default_table>
class impl;

template <class... Traits, class Storage, class Table>
class impl<interface<Traits...>, Storage, Table> {
  template <class T, class Param, class Arg>
  friend constexpr auto detail::unwrap_(Arg &&arg) -> decltype(auto);

  using storage_type = Storage;
  using destroys = detail::destroys_<storage_type>;
  using interface_type = interface<Traits..., destroys>;
  using table_type = Table::template type<interface_type, impl>;

  table_type table;
  storage_type storage;

public:
  template <implements<interface_type> T, detail::invocable_r<T> Fn>
  constexpr impl(std::in_place_type_t<T> type, Fn &&fn)
      : table(table_type::template make<T>()),
        storage(std::move(type), std::forward<Fn>(fn)) {}

  template <implements<interface_type> T, class... Args>
    requires std::constructible_from<T, Args &&...>
  constexpr impl(std::in_place_type_t<T> type, Args &&...args)
      : impl(type, detail::constructor_{type, std::forward<Args>(args)...}) {}

  template <implements<interface_type> T>
  constexpr impl(T &&object)
      : impl(std::in_place_type<T>, std::forward<T>(object)) {}

  impl(const impl &) = delete;
  impl(impl &&) = delete;

  template <detail::trait_of_<interface_type, impl> Trait>
  constexpr auto operator[](Trait trait) const {
    return table[std::move(trait)];
  }

  constexpr ~impl() { table[destroys{}](storage); }
};

template <class Interface, class Storage, class Table>
inline constexpr bool detail::is_impl_<impl<Interface, Storage, Table>> = true;

} // namespace pr
