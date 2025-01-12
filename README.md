# pr::any_view

```cpp
#include <pr/any_view.hpp>

#include <array>
#include <vector>

static_assert([] {
  using enum pr::any_kind;

  pr::any_view<int, random_access | sized> view;
  std::array arr{10, 20, 30, 40, 50};
  std::vector vec{1, 2, 3};

  view = arr;
  view[4] = 42;
  view = vec;
  view[0] = 7;

  return arr[4] == 42 and vec[0] == 7 and view.size() == 3;
}());
```

## Features:
- C++23 implementation (uses `if consteval`, `std::unreachable()`, and `auto`-cast)
- Fully constexpr and SFINAE-friendly
- Supports:
  * Ranges of any iterator category including `contiguous_range<R>`
  * `borrowed_range<R>`, `range<const R>`, `copyable<R>`, and `sized_range<R>`
- No allocation performed for small view and iterator types
