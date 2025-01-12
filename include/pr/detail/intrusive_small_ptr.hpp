#pragma once

#include <pr/detail/concepts.hpp>

#include <utility>

namespace pr::detail {

enum class index_type : bool {
  is_inplace,
  is_pointer,
};

template <class InterfaceT, class InplaceT>
  requires std::has_virtual_destructor_v<InterfaceT>
class intrusive_small_ptr {
  union {
    // declaring inplace storage as mutable allows "shallow const" semantics
    mutable InplaceT inplace_;
    InterfaceT *pointer_;
  };

  index_type index_;

public:
  constexpr intrusive_small_ptr() noexcept
      : pointer_(nullptr), index_(index_type::is_pointer) {}

  template <std::derived_from<InterfaceT> AdaptorT, class... ArgsT>
    requires std::constructible_from<AdaptorT, ArgsT...>
  constexpr intrusive_small_ptr(std::in_place_type_t<AdaptorT> tag
                                [[maybe_unused]],
                                ArgsT &&...args) {
    if constexpr (sizeof(AdaptorT) <= sizeof(InplaceT) and
                  alignof(AdaptorT) <= alignof(InplaceT)) {
      // placement new is not allowed in a constant expression
      if (not std::is_constant_evaluated()) {
        ::new (&inplace_) AdaptorT(static_cast<ArgsT &&>(args)...);
        index_ = index_type::is_inplace;
        return;
      }
    }

    pointer_ = new AdaptorT(static_cast<ArgsT &&>(args)...);
    index_ = index_type::is_pointer;
  }

  constexpr intrusive_small_ptr(const intrusive_small_ptr &other)
    requires interface_copyable<InterfaceT>
      : index_(other.index_) {
    switch (index_) {
    case index_type::is_inplace:
      static_cast<InterfaceT *>(static_cast<void *>(&other.inplace_))
          ->copy_to(&inplace_);
      break;
    default:
      pointer_ = other.pointer_->copy();
      break;
    }
  }

  constexpr intrusive_small_ptr(intrusive_small_ptr &&other) noexcept
      : index_(other.index_) {
    switch (index_) {
    case index_type::is_inplace:
      static_cast<InterfaceT *>(static_cast<void *>(&other.inplace_))
          ->move_to(&inplace_);
      break;
    case index_type::is_pointer:
      // optimization: skip move construction of allocated subobject
      pointer_ = std::exchange(other.pointer_, nullptr);
      break;
    }
  }

  constexpr auto
  operator=(const intrusive_small_ptr &other) -> intrusive_small_ptr &
    requires interface_copyable<InterfaceT>
  {
    // prevent self-assignment
    if (this == &other) {
      return *this;
    }

    this->~intrusive_small_ptr();
    std::construct_at(this, other);
    return *this;
  }

  constexpr auto
  operator=(intrusive_small_ptr &&other) noexcept -> intrusive_small_ptr & {
    this->~intrusive_small_ptr();
    std::construct_at(this, static_cast<intrusive_small_ptr &&>(other));
    return *this;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return index_ == index_type::is_inplace or pointer_ != nullptr;
  }

  [[nodiscard]] constexpr auto get() const noexcept -> InterfaceT * {
    switch (index_) {
    case index_type::is_inplace:
      return static_cast<InterfaceT *>(static_cast<void *>(&inplace_));
    case index_type::is_pointer:
      return pointer_;
    }
  }

  [[nodiscard]] constexpr auto operator*() const noexcept -> InterfaceT & {
    return *get();
  }

  [[nodiscard]] constexpr auto operator->() const noexcept -> InterfaceT * {
    return get();
  }

  constexpr ~intrusive_small_ptr() noexcept {
    switch (index_) {
    case index_type::is_inplace:
      static_cast<InterfaceT *>(static_cast<void *>(&inplace_))->~InterfaceT();
      break;
    case index_type::is_pointer:
      // deleting nullptr is a no-op, so no need to check
      delete pointer_;
      break;
    }

    // allows destructor to be called multiple times
    pointer_ = nullptr;
    index_ = index_type::is_pointer;
  }
};

} // namespace pr::detail
