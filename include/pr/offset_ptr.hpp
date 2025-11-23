#pragma once

#include <memory>

namespace pr {
namespace is {

template <class T>
concept element = std::is_object_v<T> or std::is_void_v<T>;

template <class From, class To>
concept pointer_convertible_to =
    requires(From *from) { static_cast<To *>(from); };

template <class From, class To>
concept implicitly_pointer_convertible_to =
    pointer_convertible_to<From, To> and
    requires(From *from, void fn(To *)) { fn(from); };

template <class From, class To>
concept explicitly_pointer_convertible_to =
    pointer_convertible_to<From, To> and
    not implicitly_pointer_convertible_to<From, To>;

template <class From, class To>
concept narrowing_to = sizeof(From) > sizeof(To);

} // namespace is

struct non_null_t {
  explicit non_null_t() = default;
};

inline constexpr non_null_t non_null{};

template <class Derived>
class offset_ptr_interface;

template <is::element T, std::signed_integral Difference = std::ptrdiff_t,
          std::integral Offset = std::uintptr_t,
          std::size_t Align = alignof(Offset), Offset Null = 1>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class offset_ptr : public offset_ptr_interface<
                       offset_ptr<T, Difference, Offset, Align, Null>> {
  friend offset_ptr_interface<offset_ptr>;

  alignas(Align) Offset offset{null_offset};

  [[nodiscard]] auto offset_from(non_null_t /**/, T *other) const noexcept
      -> Offset {
    return static_cast<Offset>(std::bit_cast<std::uintptr_t>(other) -
                               std::bit_cast<std::uintptr_t>(this));
  }

  [[nodiscard]] constexpr auto offset_from(T *other) const noexcept -> Offset {
    return other ? offset_from(non_null, other) : null_offset;
  }

  [[nodiscard]] constexpr auto
  offset_from(const offset_ptr &other) const noexcept -> Offset {
    return other ? offset_from(non_null, std::to_address(other)) : null_offset;
  }

public:
  using element_type = T;
  using pointer = T *;
  using offset_type = Offset;

  template <class U>
  using rebind = offset_ptr<U, Difference, Offset, Align, Null>;

  static auto pointer_to(element_type &other) noexcept -> offset_ptr {
    return offset_ptr(non_null, std::addressof(other));
  }

  static constexpr offset_type null_offset = Null;

  offset_ptr() = default;

  constexpr offset_ptr(std::nullptr_t) noexcept {}

  constexpr explicit offset_ptr(
      is::pointer_convertible_to<T> auto *other) noexcept
      : offset(offset_from(other)) {}

  explicit offset_ptr(non_null_t non_null,
                      is::pointer_convertible_to<T> auto *other) noexcept
      : offset(offset_from(non_null, other)) {}

  constexpr offset_ptr(const offset_ptr &other) noexcept
      : offset(offset_from(other)) {}

  template <is::pointer_convertible_to<T> U, class D, class O, std::size_t A,
            O N>
  constexpr explicit(is::explicitly_pointer_convertible_to<U, T> or
                     is::narrowing_to<O, Offset>)
      offset_ptr(const offset_ptr<U, D, O, A, N> &other) noexcept
      : offset(offset_from(other)) {}

  template <is::pointer_convertible_to<T> U, class D, class O, std::size_t A,
            O N>
  explicit offset_ptr(non_null_t non_null,
                      const offset_ptr<U, D, O, A, N> &other) noexcept
      : offset(offset_from(non_null, std::to_address(other))) {}

  constexpr auto operator=(std::nullptr_t) noexcept -> offset_ptr & {
    offset = null_offset;
    return *this;
  }

  constexpr auto operator=(pointer other) noexcept -> offset_ptr & {
    offset = offset_from(other);
    return *this;
  }

  constexpr auto operator=(const offset_ptr &other) noexcept -> offset_ptr & {
    offset = offset_from(other);
    return *this;
  }

  constexpr void swap(offset_ptr &other) noexcept {
    const auto new_offset_for_this = offset_from(other);
    other.offset = other.offset_from(*this);
    offset = new_offset_for_this;
  }

  friend constexpr void swap(offset_ptr &x, offset_ptr &y) noexcept {
    x.swap(y);
  }

  ~offset_ptr() = default;

  [[nodiscard]] auto operator->() const noexcept -> pointer {
    return std::bit_cast<T *>(std::bit_cast<std::uintptr_t>(this) + offset);
  }

  [[nodiscard]] constexpr auto get() const noexcept -> pointer {
    return *this ? std::to_address(*this) : nullptr;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return offset != null_offset;
  }

  template <class U, class D, class O, std::size_t A, O N>
  [[nodiscard]] constexpr auto
  operator<=>(const offset_ptr<U, D, O, A, N> &other) const noexcept
      -> std::weak_ordering {
    return get() <=> other.get();
  }

  template <class U, class D, class O, std::size_t A, O N>
  [[nodiscard]] constexpr auto
  operator==(const offset_ptr<U, D, O, A, N> &other) const noexcept -> bool {
    return get() == other.get();
  }

  [[nodiscard]] constexpr auto operator<=>(std::nullptr_t) const noexcept
      -> std::weak_ordering {
    return get() <=> static_cast<pointer>(nullptr);
  }

  [[nodiscard]] constexpr auto operator==(std::nullptr_t) const noexcept
      -> bool {
    return not *this;
  }
};

template <class T>
offset_ptr(T *) -> offset_ptr<T>;

template <class T, class Difference, class Offset, std::size_t Align,
          Offset Null>
  requires std::is_void_v<T>
class offset_ptr_interface<offset_ptr<T, Difference, Offset, Align, Null>> {};

template <class T, class Difference, class Offset, std::size_t Align,
          Offset Null>
  requires std::is_object_v<T>
class offset_ptr_interface<offset_ptr<T, Difference, Offset, Align, Null>> {
  using derived_type = offset_ptr<T, Difference, Offset, Align, Null>;

  [[nodiscard]] auto derived() noexcept -> derived_type & {
    return static_cast<derived_type &>(*this);
  }

  [[nodiscard]] auto derived() const noexcept -> const derived_type & {
    return static_cast<const derived_type &>(*this);
  }

public:
  using reference = T &;
  using difference_type = Difference;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::contiguous_iterator_tag;

  [[nodiscard]] auto operator*() const noexcept -> reference {
    return *std::to_address(derived());
  }

  [[nodiscard]] auto operator[](difference_type n) const noexcept -> reference {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return std::to_address(derived())[n];
  }

  auto operator+=(difference_type n) noexcept -> derived_type & {
    auto &self = derived();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    self.offset = self.offset_from(non_null, std::to_address(self) + n);
    /* Do not use the line below; it is a pessimization which
     * actually obscures pointer provenance from the compiler */
    // self.offset += n * sizeof(T);
    return self;
  }

  auto operator-=(difference_type n) noexcept -> derived_type & {
    auto &self = derived();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    self.offset = self.offset_from(non_null, std::to_address(self) - n);
    /* Do not use the line below; it is a pessimization which
     * actually obscures pointer provenance from the compiler */
    // self.offset -= n * sizeof(T);
    return self;
  }

  [[nodiscard]] auto operator+(difference_type n) const noexcept
      -> derived_type {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return derived_type(non_null, std::to_address(derived()) + n);
  }

  [[nodiscard]] friend auto operator+(difference_type n,
                                      const derived_type &self) noexcept
      -> derived_type {
    return self + n;
  }

  [[nodiscard]] auto operator-(difference_type n) const noexcept
      -> derived_type {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return derived_type(non_null, std::to_address(derived()) - n);
  }

  [[nodiscard]] auto operator-(const derived_type &other) const noexcept
      -> difference_type {
    return static_cast<difference_type>(std::to_address(derived()) -
                                        std::to_address(other));
  }

  auto operator++() noexcept -> derived_type & { return derived() += 1; }

  [[nodiscard]] auto operator++(int) noexcept -> derived_type {
    auto &self = derived();
    auto other = self;
    ++self;
    return other;
  }

  auto operator--() noexcept -> derived_type & { return derived() -= 1; }

  [[nodiscard]] auto operator--(int) noexcept -> derived_type {
    auto &self = derived();
    auto other = self;
    --self;
    return other;
  }
};

} // namespace pr
