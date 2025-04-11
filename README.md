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

## [`include/pr/simpl.hpp`](include/pr/simpl.hpp)

<details>
<summary><h3 style="display:inline-block">Synopsis</h3></summary>

```cpp
namespace pr {

template <typename T, template <typename...> typename L>
concept /*type-specialization-of*/ = /* see below */;

template <typename T, template <auto...> typename L>
concept /*value-specialization-of*/ = /* see below */;

struct trait_base {};

template <typename T>
concept trait = std::is_aggregate_v<T> and std::is_empty_v<T> and
                std::is_base_of_v<pr::trait_base, T>;

template <pr::trait... Traits>
struct interface;

template <typename T>
concept /*interface*/ = /*type-specialization-of*/<T, pr::interface>;

template <typename T>
concept /*table*/ = /* see below */;

struct inplace_table;

struct pointer_table;

template <typename T>
concept /*storage*/ = /* see below */;

template <typename T, typename Storage>
concept /*small*/ = /* see below */;

template <typename Fn, typename R, typename... Args>
concept /*invocable-r*/ = std::is_invocable_r_v<R, Fn, Args...>;

template <typename T>
concept /*reference*/ = std::is_reference_v<T>;

template <typename T>
concept /*non-reference*/ = not /*reference*/<T>;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct inplace_storage;

struct pointer_storage;

template <std::size_t Size, std::size_t Align = alignof(void *)>
struct small_storage;

template <typename T>
concept /*inplace-storage*/ =
    /*storage*/<T> and /*value-specialization-of*/<T, inplace_storage>;

template <typename T>
concept /*pointer-storage*/ = /*storage*/<T> and std::same_as<T, pointer_storage>;

template <typename T>
concept /*small-storage*/ =
    /*storage*/<T> and /*value-specialization-of*/<T, small_storage>;

template </*interface*/ Interface,
          /*storage*/ Storage = pr::small_storage<sizeof(void *)>,
          /*table*/ Table = pr::pointer_table>
class impl;

template <typename T>
concept /*impl*/ = /*type-specialization-of*/<T, pr::impl>;

template <typename T, typename Trait>
concept declares = /*impl*/<T> and pr::trait<Trait>;

template <typename T, typename Trait>
concept implements = not /*impl*/<std::remove_cvref_t<T>> and
                     pr::trait<Trait> and Trait::template enable<T>;

template <typename T, typename U>
concept /*different-from*/ =
    not std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename T, typename Wrapper, typename Interface>
concept concrete = /*different-from*/<T, Wrapper> and /*interface*/<Interface> and
                   pr::implements<T, Interface>;

template <typename T, typename Storage>
concept /*storable*/ = /* see below */;

template <typename Trait, typename Impl>
concept /*trait-of*/ = /* see below */;

template </*interface*/ Interface, /*storage*/ Storage, /*table*/ Table>
class impl {
public:
  using interface_type = Interface;
  using storage_type = Storage;
  using table_type = /* see below */;

private:
  table_type m_table; // exposition-only
  storage_type m_storage; // exposition-only

public:
  template <pr::implements<interface_type> T, /*invocable-r*/<T> F>
    requires /*inplace-storage*/<storage_type> and /*small*/<T, storage_type>
  impl(std::in_place_type_t<T> type, F &&f) noexcept(/* see below */);

  template <pr::implements<interface_type> T, /*invocable-r*/<T> F>
    requires /*pointer-storage*/<storage_type> or
                 /*small-storage*/<storage_type>
  constexpr impl(std::in_place_type_t<T> type, F &&f);

  template <pr::implements<interface_type> T, /*invocable-r*/<T> F>
    requires /*small-storage*/<storage_type> and
                 /*small*/<T, storage_type> and /*non-reference*/<T>
  constexpr impl(std::in_place_type_t<T> type, F &&f);

  template <pr::implements<interface_type> T, typename... Args>
    requires /*storable*/<T, storage_type> and
             std::constructible_from<T, Args &&...>
  constexpr impl(std::in_place_type_t<T> type, Args &&...args) noexcept(
      /* see below */);

  template <pr::implements<interface_type> T>
    requires /*storable*/<T, storage_type> and std::constructible_from<T, T &&>
  constexpr impl(T &&object) noexcept(/* see below */);

  impl(const impl &) = delete;
  impl(impl &&) = delete;

  auto operator=(const impl &) -> impl & = delete;
  auto operator=(impl &&) -> impl & = delete;

  constexpr auto operator[](/*trait-of*/<impl> auto trait) const noexcept;

  constexpr ~impl() noexcept(/*inplace-storage*/<storage_type>);
};

} // namespace pr
```

</details>

<details>
<summary><h3 style="display:inline-block">Example</h3></summary>

```cpp
#include <pr/simpl.hpp>

struct copy_constructs : pr::trait_base {
  template <std::copy_constructible T>
  static constexpr bool enable = true;

  template <pr::declares<copy_constructs> T>
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

  template <pr::declares<speaks> T>
  static constexpr auto fn(const T &object) -> std::string;

  template <pr::implements<speaks> T>
  static constexpr auto fn(const T &object) -> std::string {
    return object.speak();
  }
};

class animal {
  using interface_type = pr::interface<copy_constructs, speaks>;

  pr::impl<interface_type> impl;

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
