# `pr`

An assortment of standalone C++ utilities

## [`include/pr/shared_view.hpp`](include/pr/shared_view.hpp)

<details>
<summary><h3 style="display:inline-block">Synopsis</h3></summary>

```cpp
namespace pr {
namespace ranges {

template <class T>
concept copyable_view = /* see description */;

template <std::ranges::view ViewT>
  requires(not std::copyable<ViewT>)
class shared_view : public std::ranges::view_interface<shared_view<ViewT>> {
  std::shared_ptr<ViewT> base_; // exposition-only

public:
  shared_view() requires std::default_initializable<ViewT>;

  explicit shared_view(ViewT base);

  auto begin() -> std::ranges::iterator_t<ViewT>;

  auto end() -> std::ranges::sentinel_t<ViewT>;
};

template <std::ranges::viewable_range RangeT>
shared_view(RangeT &&) -> shared_view<std::views::all_t<RangeT>>;

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
    std::copyable<T> and std::ranges::view<T>;

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
constexpr auto shared(RangeT &&range) -> std::ranges::view auto;
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

| Member object     | Definition                                                                   |
| ----------------- | ---------------------------------------------------------------------------- |
| `base_` (private) | A `std::shared_ptr` of the underlying view. (exposition-only member object*) |

</details>

<details>
<summary><h4 style="display:inline-block">Member functions</h4></summary>

#### `pr::ranges::shared_view<ViewT>::shared_view`

| <!-- -->                                                    | <!-- --> |
| ----------------------------------------------------------- | -------- |
| `shared_view() requires std::default_initializable<ViewT>;` | (1)      |
| `explicit shared_view(ViewT base);`                         | (2)      |

Constructs a `shared_view`.

1) Default constructor. Initializes `base_` as if by `base_(std::make_shared<ViewT>())`.
2) Initializes the underlying `base_` with `std::make_shared<ViewT>(std::move(base))`.

---

#### `pr::ranges::shared_view<ViewT>::begin`

| <!-- -->                                          |
| ------------------------------------------------- |
| `auto begin() -> std::ranges::iterator_t<ViewT>;` |

Equivalent to `return std::ranges::begin(*base_);`. 

---

#### `pr::ranges::shared_view<ViewT>::end`

| <!-- -->                                        |
| ----------------------------------------------- |
| `auto end() -> std::ranges::iterator_t<ViewT>;` |

Equivalent to `return std::ranges::end(*base_);`. 

</details>

<details>
<summary><h4 style="display:inline-block">Deduction guides</h4></summary>

```cpp
template <std::ranges::viewable_range RangeT>
shared_view(RangeT &&) -> shared_view<std::views::all_t<RangeT>>;
```

The deduction guide is provided for `pr::ranges::shared_view` to allow deduction from `range`. 

</details>

<details>
<summary><h4 style="display:inline-block">Helper templates</h4></summary>

```cpp
template <class T>
inline constexpr bool
    std::ranges::enable_borrowed_range<pr::ranges::shared_view<T>> =
        std::ranges::enable_borrowed_range<T>;
```

This specialization of `std::ranges::enable_borrowed_range` makes `shared_view` satisfy `borrowed_range` when the underlying view satisfies it. 

</details>
</details>
