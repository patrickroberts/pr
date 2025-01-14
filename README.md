# any_view

```cpp
#include <pr/any_view.hpp>

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <vector>

constexpr auto sum(pr::any_view<const int> view) {
  return std::ranges::fold_left(view, 0, std::plus{});
}

static_assert(150 == sum(std::array{10, 20, 30, 40, 50}));
static_assert(100 == sum(std::ranges::views::repeat(20, 5)));
static_assert(75 == sum(std::span<const int>{{5, 10, 15, 20, 25}}));
static_assert(15 == sum(std::vector{1, 2, 3, 4, 5}));
```

## Features

- C++23 implementation
- Clang and GCC support (MSVC support planned)
- Fully constexpr and SFINAE-friendly
- Type-erased interfaces for
  * all input range categories including `contiguous_range<R>`
  * `borrowed_range<R>`, `common_range<R>`, `copyable<R>`, `range<const R>`, and `sized_range<R>`
- No allocation performed for small view and iterator types
