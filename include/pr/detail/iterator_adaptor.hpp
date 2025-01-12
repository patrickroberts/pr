#pragma once

#include <pr/detail/iterator_interface.hpp>

namespace pr::detail {

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
class iterator_adaptor;

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT>
class input_iterator_adaptor
    : public iterator_interface<ElementT, KindV, ReferenceT, DifferenceT> {
public:
  using reference = ReferenceT;

  [[no_unique_address]] IteratorT iterator_;

  constexpr input_iterator_adaptor()
    requires std::default_initializable<IteratorT>
  = default;

  constexpr input_iterator_adaptor(IteratorT &&iterator)
      : iterator_(static_cast<IteratorT &&>(iterator)) {}

  [[nodiscard]] constexpr auto operator*() const -> reference override {
    return *iterator_;
  }

  constexpr auto operator++() -> void override { ++iterator_; }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
class forward_iterator_adaptor
    : public input_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                    IteratorT> {
  using interface_ =
      iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>;
  using base_ = input_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                       IteratorT>;

public:
  [[no_unique_address]] SentinelT sentinel_;

  constexpr explicit forward_iterator_adaptor()
    requires std::default_initializable<IteratorT> and
                 std::default_initializable<SentinelT>
  = default;

  constexpr explicit forward_iterator_adaptor(IteratorT &&iterator,
                                              SentinelT &&sentinel)
      : base_(static_cast<IteratorT &&>(iterator)),
        sentinel_(static_cast<SentinelT &&>(sentinel)) {}

  [[nodiscard]] constexpr auto
  operator==(const interface_ &other) const -> bool override {
    if (typeid(IteratorT) != other.type()) [[unlikely]] {
      std::unreachable();
    }

    return this->iterator_ == static_cast<const base_ &>(other).iterator_;
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
  requires disables_kind<KindV, any_kind::forward>
class forward_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                               IteratorT, SentinelT>
    : public input_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                    IteratorT> {
  using base_ = input_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                       IteratorT>;

  [[no_unique_address]] SentinelT sentinel_;

public:
  constexpr forward_iterator_adaptor(IteratorT &&iterator, SentinelT &&sentinel)
      : base_(static_cast<IteratorT &&>(iterator)),
        sentinel_(static_cast<SentinelT &&>(sentinel)) {}
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
class bidirectional_iterator_adaptor
    : public forward_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                      IteratorT, SentinelT> {
public:
  using bidirectional_iterator_adaptor::forward_iterator_adaptor::
      forward_iterator_adaptor;

  constexpr auto operator--() -> void override { --this->iterator_; }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
  requires disables_kind<KindV, any_kind::bidirectional>
class bidirectional_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                     IteratorT, SentinelT>
    : public forward_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                      IteratorT, SentinelT> {
public:
  using bidirectional_iterator_adaptor::forward_iterator_adaptor::
      forward_iterator_adaptor;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
class random_access_iterator_adaptor
    : public bidirectional_iterator_adaptor<ElementT, KindV, ReferenceT,
                                            DifferenceT, IteratorT, SentinelT> {
  using interface_ =
      iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>;
  using base_ = input_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                       IteratorT>;

public:
  using reference = ReferenceT;
  using difference_type = DifferenceT;

  using random_access_iterator_adaptor::bidirectional_iterator_adaptor::
      bidirectional_iterator_adaptor;

  [[nodiscard]] constexpr auto
  operator<=>(const interface_ &other) const -> std::partial_ordering override {
    if (typeid(IteratorT) != other.type()) {
      std::unreachable();
    }

    return this->iterator_ <=> static_cast<const base_ &>(other).iterator_;
  }

  [[nodiscard]] constexpr auto
  operator-(const interface_ &other) const -> difference_type override {
    if (typeid(IteratorT) != other.type()) {
      std::unreachable();
    }

    return this->iterator_ - static_cast<const base_ &>(other).iterator_;
  }

  constexpr auto operator+=(difference_type offset) -> void override {
    this->iterator_ += offset;
  }

  constexpr auto operator-=(difference_type offset) -> void override {
    this->iterator_ -= offset;
  }

  [[nodiscard]] constexpr auto
  operator[](difference_type offset) const -> reference override {
    return this->iterator_[offset];
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
  requires disables_kind<KindV, any_kind::random_access>
class random_access_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                     IteratorT, SentinelT>
    : public bidirectional_iterator_adaptor<ElementT, KindV, ReferenceT,
                                            DifferenceT, IteratorT, SentinelT> {
public:
  using random_access_iterator_adaptor::bidirectional_iterator_adaptor::
      bidirectional_iterator_adaptor;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
class contiguous_iterator_adaptor
    : public random_access_iterator_adaptor<ElementT, KindV, ReferenceT,
                                            DifferenceT, IteratorT, SentinelT> {
public:
  using pointer = std::add_pointer_t<ReferenceT>;

  using contiguous_iterator_adaptor::random_access_iterator_adaptor::
      random_access_iterator_adaptor;

  [[nodiscard]] constexpr auto operator->() const -> pointer override {
    return std::to_address(this->iterator_);
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
  requires disables_kind<KindV, any_kind::contiguous>
class contiguous_iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT,
                                  IteratorT, SentinelT>
    : public random_access_iterator_adaptor<ElementT, KindV, ReferenceT,
                                            DifferenceT, IteratorT, SentinelT> {
public:
  using contiguous_iterator_adaptor::random_access_iterator_adaptor::
      random_access_iterator_adaptor;
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
class iterator_adaptor final
    : public contiguous_iterator_adaptor<ElementT, KindV, ReferenceT,
                                         DifferenceT, IteratorT, SentinelT> {
  using interface_ =
      iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>;

public:
  using iterator_adaptor::contiguous_iterator_adaptor::
      contiguous_iterator_adaptor;

  constexpr iterator_adaptor(const iterator_adaptor &) = default;
  constexpr iterator_adaptor(iterator_adaptor &&) noexcept = default;

  [[nodiscard]] constexpr auto operator==(
      [[maybe_unused]] std::default_sentinel_t other) const -> bool override {
    return this->iterator_ == this->sentinel_;
  }

  auto copy_to(void *destination) const -> void override {
    ::new (destination)
        iterator_adaptor(static_cast<const iterator_adaptor &>(*this));
  }

  [[nodiscard]] constexpr auto copy() const -> interface_ * override {
    return new iterator_adaptor(*static_cast<const iterator_adaptor *>(
        static_cast<const void *>(this)));
  }

  auto move_to(void *destination) noexcept -> void override {
    ::new (destination)
        iterator_adaptor(static_cast<iterator_adaptor &&>(*this));
  }

  [[nodiscard]] constexpr auto
  type() const noexcept -> const std::type_info & override {
    return typeid(IteratorT);
  }
};

template <class ElementT, any_kind KindV, class ReferenceT, class DifferenceT,
          class IteratorT, class SentinelT>
  requires disables_kind<KindV, any_kind::forward>
class iterator_adaptor<ElementT, KindV, ReferenceT, DifferenceT, IteratorT,
                       SentinelT>
    final
    : public contiguous_iterator_adaptor<ElementT, KindV, ReferenceT,
                                         DifferenceT, IteratorT, SentinelT> {
public:
  using iterator_adaptor::contiguous_iterator_adaptor::
      contiguous_iterator_adaptor;

  constexpr iterator_adaptor(const iterator_adaptor &) = default;
  constexpr iterator_adaptor(iterator_adaptor &&) noexcept = default;

  [[nodiscard]] constexpr auto operator==(
      [[maybe_unused]] std::default_sentinel_t other) const -> bool override {
    return this->iterator_ == this->sentinel_;
  }

  auto move_to(void *destination) noexcept -> void override {
    ::new (destination)
        iterator_adaptor(static_cast<iterator_adaptor &&>(*this));
  }

  [[nodiscard]] constexpr auto
  type() const noexcept -> const std::type_info & override {
    return typeid(IteratorT);
  }
};

} // namespace pr::detail
