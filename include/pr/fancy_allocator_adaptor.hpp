#pragma once

#include <memory>

namespace pr {
namespace detail {

template <class AllocPtr, class FancyPtr>
concept adaptable_with =
    std::constructible_from<FancyPtr, AllocPtr> and requires(FancyPtr p) {
      {
        std::pointer_traits<AllocPtr>::pointer_to(*p)
      } -> std::same_as<AllocPtr>;
    };

} // namespace detail

template <class Allocator, class Pointer>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class fancy_allocator_adaptor : public Allocator {
  using alloc_traits = std::allocator_traits<Allocator>;
  using alloc_ptr = alloc_traits::pointer;
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

  [[nodiscard]] constexpr auto allocate(size_type n) -> pointer
    requires detail::adaptable_with<alloc_ptr, pointer>
  {
    return pointer(alloc_traits::allocate(*this, n));
  }

  constexpr void deallocate(const pointer &p, size_type n)
    requires detail::adaptable_with<alloc_ptr, pointer>
  {
    alloc_traits::deallocate(*this,
                             std::pointer_traits<alloc_ptr>::pointer_to(*p), n);
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

  [[nodiscard]] constexpr auto
  operator==(const fancy_allocator_adaptor &other) const noexcept -> bool {
    return static_cast<const Allocator &>(*this) ==
           static_cast<const Allocator &>(other);
  }

  template <class Alloc, class Ptr>
    requires std::equality_comparable_with<Allocator, Alloc>
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
