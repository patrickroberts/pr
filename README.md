# `pr`

An assortment of standalone C++ utilities

## [`include/pr/context.hpp`](include/pr/context.hpp)

<details>
<summary><h3 style="display:inline-block">Synopsis</h3></summary>

```cpp
namespace pr {

template <class T>
concept /*storable*/ = std::same_as<T, std::remove_cvref_t<T>>;

template </*storable*/ T>
inline thread_local constinit T * /*context*/ = nullptr;

template <class T>
concept /*makeable*/ =
    /*storable*/<std::remove_reference_t<T>> and not std::is_rvalue_reference_v<T>;

template <makeable_ T>
class /*provider*/ {
  using value_type = std::remove_reference_t<T>;

  // `mutable` prevents UB when `make_context` initializes a `const auto`
  [[no_unique_address]] mutable T inner_; // exposition-only
  value_type *outer_; // exposition-only

public:
  template <class... ArgsT>
    requires std::constructible_from<T, ArgsT...>
  explicit /*provider*/(ArgsT &&...args) noexcept(
      std::is_nothrow_constructible_v<T, ArgsT...>);

  /*provider*/(const /*provider*/ &) = delete;
  /*provider*/(/*provider*/ &&) = delete;

  ~/*provider*/() noexcept;
};

template </*makeable*/ T, class... ArgsT>
  requires std::constructible_from<T, ArgsT...>
[[nodiscard]] auto make_context(ArgsT &&...args) noexcept(
    std::is_nothrow_constructible_v<T, ArgsT...>) -> /*provider*/<T>;

template <class T>
concept /*gettable*/ = /*storable*/<std::remove_const_t<T>>;

template </*gettable*/ T>
[[nodiscard]] auto get_context() noexcept -> T *;

} // namespace pr
```

</details>

---

<details>
<summary><h3 style="display:inline-block"><code>pr::make_context</code></h3></summary>

```cpp
template </*makeable*/ T, class... ArgsT>
  requires std::constructible_from<T, ArgsT...>
[[nodiscard]] auto make_context(ArgsT &&...args) noexcept(
    std::is_nothrow_constructible_v<T, ArgsT...>) -> /*provider*/<T>;
```

Constructs and returns `/*provider*/<T>`, whose constructor initializes its members as if by `inner_(std::forward<ArgsT>(args)...), outer_(std::exchange(/*context*/<value_type>, std::addressof(inner_)))`. Its destructor restores `/*context*/<value_type>` to the value of `outer_`. `T` must be a cv-unqualified non-reference or lvalue-reference type, or the instantiation is ill-formed, which can result in substitution failure when the call appears in the immediate context of a template instantiation.

</details>

---

<details>
<summary><h3 style="display:inline-block"><code>pr::get_context</code></h3></summary>

```cpp
template </*gettable*/ T>
[[nodiscard]] auto get_context() noexcept -> T *;
```

Returns `/*context*/<value_type>`, whose value has been initialized by thread-local calls to `pr::make_context<value_type>(...)` or `pr::make_context<value_type &>(...)`, or `nullptr` otherwise. `T` must be an optionally const-qualified non-reference type, or the instantiation is ill-formed, which can result in substitution failure when the call appears in the immediate context of a template instantiation.

</details>

## [`include/pr/shared_view.hpp`](include/pr/shared_view.hpp)

<details>
<summary><h3 style="display:inline-block">Synopsis</h3></summary>

```cpp
namespace pr {
namespace ranges {

template <class T>
concept copyable_view = /* see description */;

template <std::ranges::viewable_range RangeT>
  requires std::is_object_v<RangeT>
class shared_view : public std::ranges::view_interface<shared_view<RangeT>> {
  std::shared_ptr<RangeT> base_; // exposition-only

public:
  shared_view() requires std::default_initializable<RangeT>;

  explicit shared_view(RangeT &&base);

  auto base() const noexcept -> RangeT &;

  auto begin() -> std::ranges::iterator_t<RangeT>;

  auto end() -> std::ranges::sentinel_t<RangeT>;
};

namespace views {

inline constexpr /* unspecified */ shared = /* unspecified*/;

} // namespace views
} // namespace ranges

namespace views = ranges::views;

} // namespace pr

template <class T>
inline constexpr bool
    std::ranges::enable_borrowed_range<pr::ranges::shared_view<T>> =
        std::ranges::enable_borrowed_range<T>;
```

</details>

---

<details>
<summary><h3 style="display:inline-block"><code>pr::ranges::copyable_view</code></h3></summary>

#### Concept

```cpp
namespace pr::ranges {

  template <class T>
  concept copyable_view =
    std::ranges::view<T> and std::copyable<T>;

}
```

The `pr::ranges::copyable_view` concept is a refinement of `std::ranges::view` for which `std::copyable` is satisfied.

</details>

---

<details>
<summary><h3 style="display:inline-block"><code>pr::views::shared</code></h3></summary>

#### Call signature

```cpp
template <std::ranges::viewable_range RangeT>
    requires /* see below */
constexpr auto shared(RangeT &&range) -> copyable_view auto;
```

Given an expression `e` of type `T`, the expression `pr::views::shared(e)` is expression-equivalent to:
- `std::views::all(e)`, if it is a well-formed expression and `std::views::all_t<T>` models `std::copyable`;
- `pr::ranges::shared_view{e}` otherwise.

</details>

---

<details>
<summary><h3 style="display:inline-block"><code>pr::ranges::shared_view</code></h3></summary>

<details>
<summary><h4 style="display:inline-block">Data members</h4></summary>

| Member object     | Definition                                                                    |
| ----------------- | ----------------------------------------------------------------------------- |
| `base_` (private) | A `std::shared_ptr` of the underlying range. (exposition-only member object*) |

</details>

<details>
<summary><h4 style="display:inline-block">Member functions</h4></summary>

#### `pr::ranges::shared_view<RangeT>::shared_view`

| <!-- -->                                                     | <!-- --> |
| ------------------------------------------------------------ | -------- |
| `shared_view() requires std::default_initializable<RangeT>;` | (1)      |
| `explicit shared_view(RangeT &&base);`                       | (2)      |

Constructs a `shared_view`.

1) Default constructor. Initializes `base_` as if by `base_(std::make_shared<RangeT>())`.
2) Initializes the underlying `base_` with `std::make_shared<RangeT>(std::move(base))`.

---

#### `pr::ranges::shared_view<RangeT>::base`

| <!-- -->                                 |
| ---------------------------------------- |
| `auto base() const noexcept -> RangeT &` |

Equivalent to `return *base_;`.

---

#### `pr::ranges::shared_view<RangeT>::begin`

| <!-- -->                                           |
| -------------------------------------------------- |
| `auto begin() -> std::ranges::iterator_t<RangeT>;` |

Equivalent to `return std::ranges::begin(*base_);`.

---

#### `pr::ranges::shared_view<RangeT>::end`

| <!-- -->                                         |
| ------------------------------------------------ |
| `auto end() -> std::ranges::iterator_t<RangeT>;` |

Equivalent to `return std::ranges::end(*base_);`.

</details>

<details>
<summary><h4 style="display:inline-block">Helper templates</h4></summary>

```cpp
template <class T>
inline constexpr bool
    std::ranges::enable_borrowed_range<pr::ranges::shared_view<T>> =
        std::ranges::enable_borrowed_range<T>;
```

This specialization of `std::ranges::enable_borrowed_range` makes `shared_view` satisfy `borrowed_range` when the underlying range satisfies it. 

</details>
</details>

## [`include/pr/simple.hpp`](include/pr/simple.hpp)

<details>
<summary><h3 style="display:inline-block">Synopsis</h3></summary>

```cpp
namespace pr {

template <typename T, typename Policy, typename... Ts>
inline constexpr bool /*enable*/ = false;

template <typename T>
concept trait = std::is_aggregate_v<T> and std::is_empty_v<T> and
                /*enable*/<T, /*is-trait*/>;

template <typename T>
concept /*interface*/ = /*enable*/<T, /*is-interface*/>;

template <typename T>
concept /*inplace*/ = /*enable*/<T, /*is-inplace*/>;

template <typename T>
concept /*pointer*/ = /*enable*/<T, /*is-pointer*/>;

template <typename T>
concept /*small*/ = /*enable*/<T, /*is-small*/>;

template <typename T>
concept /*small*/ = /*inplace*/<T> or /*pointer*/<T> or /*small*/<T>;

template <typename T>
concept /*storable-to*/ = /*storage*/<T> and /* see below */;

template <typename T>
concept /*table*/ = /*enable*/<T, /*is-table*/>;

template <typename T>
concept /*simple*/ = /*enable*/<T, /*is-simple*/>;

template <typename Trait, typename Simple>
concept trait_of = pr::trait<Trait> and /*simple*/<Simple> and
                   /*enable*/<Trait, /*is-trait-of*/, Simple>;

template <typename Simple, typename Trait>
concept simplifies = pr::trait_of<Trait, Simple>;

template <typename T, typename Trait>
concept implements = not /*simple*/<std::remove_cvref_t<T>> and
                     pr::trait<Trait> and Trait::template enable<T>;

template <typename T, typename U>
concept /*different-from*/ =
    not std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename T, typename Wrapper, typename Interface>
concept concrete = /*different-from*/<T, Wrapper> and
                   /*interface*/<Interface> and implements<T, Interface>;

template <typename F, typename R, typename... Args>
concept /*invocable-r*/ = std::is_invocable_r_v<R, F, Args...>;

template <typename T>
concept /*reference*/ = std::is_reference_v<T>;

template <typename T>
concept /*prvalue*/ = not /*reference*/<T>;

template <typename T, typename Storage>
concept /*inplace-storable-to*/ =
    /*storage*/<Storage> and not /*pointer*/<Storage> and
    /* see below */;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct inplace_storage;

struct pointer_storage;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct small_storage;

struct trait_base {};

template <pr::trait... Traits>
struct interface;

struct move_constructs;

struct inplace_table;

struct pointer_table;

template </*interface*/ Interface,
          /*storage*/ Storage = pr::small_storage<sizeof(void *)>,
          /*table*/ Table = pr::pointer_table>
class simple {
public:
  using interface_type = Interface;
  using storage_type = Storage;
  using table_type = /* unspecified */;

private:
  table_type m_table; // exposition-only
  storage_type m_storage; // exposition-only

public:
  template <pr::implements<interface_type> T, /*invocable-r*/<T> F>
    requires /*inplace*/<storage_type> and
                 /*inplace-storable-to*/<T, storage_type>
  simple(std::in_place_type_t<T> tag,
         F &&func) noexcept(std::is_nothrow_invocable_r_v<T, F>);

  template <pr::implements<interface_type> T, /*invocable-r*/<T> F>
    requires /*pointer*/<storage_type> or /*small*/<storage_type>
  constexpr simple(std::in_place_type_t<T> tag,
                   F &&func) noexcept(std::is_nothrow_invocable_r_v<T, F> and
                                      /*reference*/<T>);

  template <pr::implements<interface_type> T, /*invocable-r*/<T> F>
    requires /*small*/<storage_type> and
                 /*inplace-storable-to*/<T, storage_type> and /*prvalue*/<T>
  constexpr simple(std::in_place_type_t<T> tag,
                   F &&func) noexcept(std::is_nothrow_invocable_r_v<T, F>);

  template <pr::implements<interface_type> T, typename... Args>
    requires /*storable-to*/<T, storage_type> and
             std::constructible_from<T, Args &&...>
  constexpr simple(std::in_place_type_t<T> tag, Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args &&...> and
      /* see below */);

  template <pr::implements<interface_type> T>
    requires /*storable-to*/<T, storage_type> and
             std::constructible_from<T, T &&>
  constexpr simple(T &&object) noexcept(
      std::is_nothrow_constructible_v<T, T &&> and /* see below */);

  template <pr::implements<interface_type> T>
    requires /*pointer*/<storage_type> or /*small*/<storage_type>
  constexpr simple(std::unique_ptr<T> &&ptr) noexcept;

  simple(const simple &) = delete;
  simple(simple &&) = delete;

  auto operator=(const simple &) -> simple & = delete;
  auto operator=(simple &&) -> simple & = delete;

  constexpr auto operator[](pr::trait_of<simple> auto trait) const noexcept;

  constexpr ~simple() noexcept(/*inplace*/<storage_type>);
};

} // namespace pr
```

</details>

<details>
<summary><h3 style="display:inline-block">Example</h3></summary>

```cpp
#include <pr/simple.hpp>

struct copy_constructs : pr::trait_base {
  template <std::copy_constructible T>
  static constexpr bool enable = true;

  template <pr::simplifies<copy_constructs> T>
  static constexpr auto fn(const T &object) -> T;

  template <pr::implements<copy_constructs> T>
  static constexpr auto fn(const T &object) -> T {
    return T(object);
  }
};

struct speaks : pr::trait_base {
  template <typename T>
  static constexpr bool enable = requires(const T &object) {
    { object.speak() } -> std::convertible_to<std::string>;
  };

  template <pr::simplifies<speaks> T>
  static constexpr auto fn(const T &object) -> std::string;

  template <pr::implements<speaks> T>
  static constexpr auto fn(const T &object) -> std::string {
    return object.speak();
  }
};

class animal {
  using interface_type = pr::interface<copy_constructs, speaks>;

  pr::simple<interface_type> impl;

public:
  template <pr::concrete<animal, interface_type> T>
  constexpr animal(T &&object) : impl(std::forward<T>(object)) {}

  constexpr animal(const animal &other)
      : impl(other.impl[copy_constructs{}](other.impl)) {}

  constexpr auto speak() const -> std::string { return impl[speaks{}](impl); }
};

struct cow {
  constexpr auto speak() const { return "moo"; }
};

struct pig {
  constexpr auto speak() const { return "oink"; }
};

struct dog {
  constexpr auto speak() const { return "woof"; }
};

// compiles with -std=c++26 using [p2738]
static_assert([] {
  std::vector<animal> animals{cow{}, pig{}, dog{}};

  return animals[0].speak() == "moo" and
         animals[1].speak() == "oink" and
         animals[2].speak() == "woof";
}());
```

</details>
