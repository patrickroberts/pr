#pragma once

#include <pr/any_iterator.hpp>

namespace pr::detail {

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class view_interface;

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class constant_view_interface {
  using iterator_ = any_iterator<ElementT, KindV & any_kind::contiguous,
                                 ReferenceT, DifferenceT>;

public:
  [[nodiscard]] constexpr virtual auto begin() const -> iterator_ = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::constant>
class constant_view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
  using iterator_ = any_iterator<ElementT, KindV & any_kind::contiguous,
                                 ReferenceT, DifferenceT>;

public:
  [[nodiscard]] constexpr virtual auto begin() -> iterator_ = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class copyable_view_interface
    : public constant_view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
  using interface_ = view_interface<ElementT, KindV, ReferenceT, DifferenceT>;

public:
  virtual auto copy_to(void *destination) const -> void = 0;

  [[nodiscard]] constexpr virtual auto copy() const -> interface_ * = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::copyable>
class copyable_view_interface<ElementT, KindV, ReferenceT, DifferenceT>
    : public constant_view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class sized_view_interface
    : public copyable_view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
public:
  using size_type = std::make_unsigned_t<DifferenceT>;

  [[nodiscard]] constexpr virtual auto size() const -> size_type = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::sized>
class sized_view_interface<ElementT, KindV, ReferenceT, DifferenceT>
    : public copyable_view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class view_interface
    : public sized_view_interface<ElementT, KindV, ReferenceT, DifferenceT> {
public:
  virtual auto move_to(void *destination) noexcept -> void = 0;

  [[nodiscard]] constexpr virtual auto
  type() const noexcept -> const std::type_info & = 0;

  constexpr virtual ~view_interface() noexcept = default;
};

} // namespace pr::detail
