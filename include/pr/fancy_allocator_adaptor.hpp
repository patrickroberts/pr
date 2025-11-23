#pragma once

#include <memory>

namespace pr {

template <class Allocator, class Pointer>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class fancy_allocator_adaptor : public Allocator {
  using alloc_traits = std::allocator_traits<Allocator>;
  using ptr_traits = std::pointer_traits<Pointer>;

public:
  using value_type = alloc_traits::value_type;
  using difference_type = alloc_traits::difference_type;
  using size_type = alloc_traits::size_type;
  using pointer = ptr_traits::template rebind<value_type>;
  using const_pointer = ptr_traits::template rebind<const value_type>;
  using void_pointer = ptr_traits::template rebind<void>;
  using const_void_pointer = ptr_traits::template rebind<const void>;

  template <class U>
  struct rebind {
    using other =
        fancy_allocator_adaptor<typename alloc_traits::template rebind_alloc<U>,
                                typename ptr_traits::template rebind<U>>;
  };

  static_assert(
      std::same_as<fancy_allocator_adaptor, typename rebind<value_type>::other>,
      "Mismatched pointer type; rebind pointer type to value_type instead");

  fancy_allocator_adaptor() = default;

  fancy_allocator_adaptor(const fancy_allocator_adaptor &other) = default;

  template <class Alloc>
    requires std::constructible_from<Allocator, Alloc>
  constexpr fancy_allocator_adaptor(Alloc &&other)
      : Allocator(std::forward<Alloc>(other)) {}

  template <class Alloc, class Ptr>
    requires std::constructible_from<Allocator, const Alloc &>
  constexpr fancy_allocator_adaptor(
      const fancy_allocator_adaptor<Alloc, Ptr> &other)
      : Allocator(static_cast<const Alloc &>(other)) {}

  auto operator=(const fancy_allocator_adaptor &other)
      -> fancy_allocator_adaptor & = default;

  ~fancy_allocator_adaptor() = default;

  [[nodiscard]] constexpr auto allocate(size_type n) -> pointer {
    return pointer(alloc_traits::allocate(*this, n));
  }

  constexpr void deallocate(pointer p, size_type n) {
    using base_pointer = alloc_traits::pointer;
    using base_traits = std::pointer_traits<base_pointer>;
    alloc_traits::deallocate(*this, base_traits::pointer_to(*p), n);
  }

  template <class T, class... Args>
  constexpr void construct(T *p, Args &&...args) {
    alloc_traits::construct(*this, p, std::forward<Args>(args)...);
  }

  template <class T>
  constexpr void destroy(T *p) {
    alloc_traits::destroy(*this, p);
  }

  [[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
    return alloc_traits::max_size(*this);
  }

  template <class Alloc, class Ptr>
  [[nodiscard]] constexpr auto
  operator==(const fancy_allocator_adaptor<Alloc, Ptr> &other) const noexcept
      -> bool {
    return static_cast<const Allocator &>(*this) ==
           static_cast<const Alloc &>(other);
  }

  [[nodiscard]] constexpr auto select_on_container_copy_construction() const
      -> fancy_allocator_adaptor {
    return fancy_allocator_adaptor(
        alloc_traits::select_on_container_copy_construction(*this));
  }
};

} // namespace pr
