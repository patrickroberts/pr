#pragma once

#include <pr/detail/view_interface.hpp>

namespace pr::detail {

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
class view_adaptor;

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
class constant_view_adaptor
    : public view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
  using iterator_ = any_iterator<ElementT, KindV & any_kind::contiguous,
                                 ReferenceT, DifferenceT>;

public:
  [[no_unique_address]] ViewT view_;

  constexpr explicit constant_view_adaptor()
    requires std::default_initializable<ViewT>
  = default;

  constexpr explicit constant_view_adaptor(ViewT &&view)
      : view_(static_cast<ViewT &&>(view)) {}

  [[nodiscard]] constexpr auto begin() const -> iterator_ override {
    return iterator_(std::ranges::begin(view_), std::ranges::end(view_));
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
  requires disables_kind<KindV, any_kind::constant>
class constant_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT, ViewT>
    : public view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
  using iterator_ = any_iterator<ElementT, KindV & any_kind::contiguous,
                                 ReferenceT, DifferenceT>;

public:
  [[no_unique_address]] ViewT view_;

  constexpr explicit constant_view_adaptor()
    requires std::default_initializable<ViewT>
  = default;

  constexpr explicit constant_view_adaptor(ViewT &&view)
      : view_(static_cast<ViewT &&>(view)) {}

  [[nodiscard]] constexpr auto begin() -> iterator_ override {
    return iterator_(std::ranges::begin(view_), std::ranges::end(view_));
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
class copyable_view_adaptor
    : public constant_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                   ViewT> {
  using interface_ = view_interface<ElementT, KindV, ReferenceT, DifferenceT>;
  using derived_ =
      view_adaptor<ElementT, KindV, ReferenceT, DifferenceT, ViewT>;

public:
  using copyable_view_adaptor::constant_view_adaptor::constant_view_adaptor;

  auto copy_to(void *destination) const -> void override {
    ::new (destination) derived_(static_cast<const derived_ &>(*this));
  }

  [[nodiscard]] constexpr auto copy() const -> interface_ * override {
    return new derived_(static_cast<const derived_ &>(*this));
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
  requires disables_kind<KindV, any_kind::copyable>
class copyable_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT, ViewT>
    : public constant_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                   ViewT> {
public:
  using copyable_view_adaptor::constant_view_adaptor::constant_view_adaptor;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
class sized_view_adaptor
    : public copyable_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                   ViewT> {
public:
  using size_type = std::make_unsigned_t<DifferenceT>;

  using sized_view_adaptor::copyable_view_adaptor::copyable_view_adaptor;

  [[nodiscard]] constexpr auto size() const -> size_type override {
    return std::ranges::size(this->view_);
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
  requires disables_kind<KindV, any_kind::sized>
class sized_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT, ViewT>
    : public copyable_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                   ViewT> {
public:
  using sized_view_adaptor::copyable_view_adaptor::copyable_view_adaptor;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class ViewT>
class view_adaptor final
    : public sized_view_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                ViewT> {
public:
  using view_adaptor::sized_view_adaptor::sized_view_adaptor;

  constexpr view_adaptor(const view_adaptor &) = default;
  constexpr view_adaptor(view_adaptor &&) noexcept = default;

  auto move_to(void *destination) noexcept -> void override {
    ::new (destination) view_adaptor(static_cast<view_adaptor &&>(*this));
  }

  [[nodiscard]] constexpr auto
  type() const noexcept -> const std::type_info & override {
    return typeid(ViewT);
  }
};

} // namespace pr::detail
