#pragma once

#include <pr/detail/intrusive_small_ptr.hpp>

#include <typeinfo>

namespace pr::detail {

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class iterator_interface;

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class input_iterator_interface {
public:
  using reference = ReferenceT;

  [[nodiscard]] constexpr virtual auto operator*() const -> reference = 0;

  constexpr virtual auto move_to(void *destination) noexcept -> void = 0;

  constexpr virtual auto operator++() -> void = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class forward_iterator_interface
    : public input_iterator_interface<ElementT, KindV, ReferenceT,
                                      DifferenceT> {
  using interface_ =
      iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>;

public:
  constexpr virtual auto copy_to(void *destination) const -> void = 0;

  [[nodiscard]] constexpr virtual auto copy() const -> interface_ * = 0;

  [[nodiscard]] constexpr virtual auto
  operator==(const interface_ &other) const -> bool = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::forward>
class forward_iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>
    : public input_iterator_interface<ElementT, KindV, ReferenceT,
                                      DifferenceT> {
  using interface_ =
      iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>;

public:
  auto operator==(const interface_ &other) const -> bool = delete;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class bidirectional_iterator_interface
    : public forward_iterator_interface<ElementT, KindV, ReferenceT,
                                        DifferenceT> {
public:
  constexpr virtual auto operator--() -> void = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::bidirectional>
class bidirectional_iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>
    : public forward_iterator_interface<ElementT, KindV, ReferenceT,
                                        DifferenceT> {};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class random_access_iterator_interface
    : public bidirectional_iterator_interface<ElementT, KindV, ReferenceT,
                                              DifferenceT> {
  using interface_ =
      iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>;

public:
  using reference = ReferenceT;
  using difference_type = DifferenceT;

  [[nodiscard]] constexpr virtual auto
  operator<=>(const interface_ &other) const -> std::partial_ordering = 0;

  [[nodiscard]] constexpr virtual auto
  operator-(const interface_ &other) const -> difference_type = 0;

  constexpr virtual auto operator+=(difference_type offset) -> void = 0;

  constexpr virtual auto operator-=(difference_type offset) -> void = 0;

  [[nodiscard]] constexpr virtual auto
  operator[](difference_type offset) const -> reference = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::random_access>
class random_access_iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>
    : public bidirectional_iterator_interface<ElementT, KindV, ReferenceT,
                                              DifferenceT> {};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class contiguous_iterator_interface
    : public random_access_iterator_interface<ElementT, KindV, ReferenceT,
                                              DifferenceT> {
public:
  using pointer = std::add_pointer_t<ReferenceT>;

  [[nodiscard]] constexpr virtual auto operator->() const -> pointer = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::contiguous>
class contiguous_iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>
    : public random_access_iterator_interface<ElementT, KindV, ReferenceT,
                                              DifferenceT> {};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class common_iterator_interface
    : public contiguous_iterator_interface<ElementT, KindV, ReferenceT,
                                           DifferenceT> {};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
  requires disables_kind<KindV, any_kind::common>
class common_iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>
    : public contiguous_iterator_interface<ElementT, KindV, ReferenceT,
                                           DifferenceT> {
public:
  using common_iterator_interface::contiguous_iterator_interface::operator==;

  [[nodiscard]] constexpr virtual auto
  operator==(std::default_sentinel_t other) const -> bool = 0;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT>
class iterator_interface
    : public common_iterator_interface<ElementT, KindV, ReferenceT,
                                       DifferenceT> {
public:
  [[nodiscard]] constexpr virtual auto
  type() const noexcept -> const std::type_info & = 0;

  constexpr virtual ~iterator_interface() noexcept = default;
};

} // namespace pr::detail
