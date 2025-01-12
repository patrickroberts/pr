#pragma once

#include <pr/detail/iterator_adaptor.hpp>

namespace pr {
namespace detail {

template <any_kind KindV>
struct iterator_concept_trait {
  using iterator_concept = std::contiguous_iterator_tag;
};

template <any_kind KindV>
  requires disables_kind<KindV, any_kind::contiguous>
struct iterator_concept_trait<KindV> {};

} // namespace detail

template <class ElementT, any_kind KindV = any_kind::input,
          class ReferenceT = std::add_lvalue_reference_t<ElementT>,
          class DifferenceT = std::ptrdiff_t>
class any_iterator : public detail::iterator_concept_trait<KindV> {
  template <class IteratorT, class SentinelT = IteratorT>
  static constexpr std::in_place_type_t<detail::iterator_adaptor<
      ElementT, KindV, ReferenceT, DifferenceT, std::decay_t<IteratorT>,
      std::decay_t<SentinelT>>>
      in_place_iterator_type_{};

  static constexpr bool forward_ =
      detail::enables_kind(KindV, any_kind::forward);
  static constexpr bool bidirectional_ =
      detail::enables_kind(KindV, any_kind::bidirectional);
  static constexpr bool random_access_ =
      detail::enables_kind(KindV, any_kind::random_access);
  static constexpr bool contiguous_ =
      detail::enables_kind(KindV, any_kind::contiguous);

  // inplace storage is large enough for vtable pointer and two T *
  using iterator_ptr_type_ = detail::intrusive_small_ptr<
      detail::iterator_interface<ElementT, KindV, ReferenceT, DifferenceT>,
      void *[3]>;
  using reference_ = ReferenceT;
  using pointer_ = std::add_pointer_t<reference_>;

  iterator_ptr_type_ iterator_ptr_;

public:
  using element_type = ElementT;
  using difference_type = DifferenceT;

  static constexpr any_kind kind = KindV;

  /// @private
  template <class IteratorT, class SentinelT>
  constexpr any_iterator(IteratorT &&iterator, SentinelT &&sentinel)
      : iterator_ptr_(in_place_iterator_type_<IteratorT, SentinelT>,
                      static_cast<std::decay_t<IteratorT>>(iterator),
                      static_cast<std::decay_t<SentinelT>>(sentinel)) {}

  any_iterator() = delete;

  constexpr any_iterator() noexcept
    requires forward_
      : iterator_ptr_(in_place_iterator_type_<pointer_>) {}

  any_iterator(const any_iterator &other) = delete;

  constexpr any_iterator(const any_iterator &other)
    requires forward_
  = default;

  constexpr any_iterator(any_iterator &&other) noexcept = default;

  auto operator=(const any_iterator &other) -> any_iterator & = delete;

  constexpr auto operator=(const any_iterator &other) -> any_iterator &
    requires forward_
  = default;

  constexpr auto
  operator=(any_iterator &&other) noexcept -> any_iterator & = default;

  [[nodiscard]] constexpr auto operator*() const -> reference_ {
    return **iterator_ptr_;
  }

  constexpr auto operator++() -> any_iterator & {
    ++*iterator_ptr_;
    return *this;
  }

  [[nodiscard]] constexpr auto operator++(int) -> any_iterator & {
    ++*iterator_ptr_;
    return *this;
  }

  [[nodiscard]] constexpr auto operator++(int) -> any_iterator
    requires forward_
  {
    const auto other = *this;
    ++*iterator_ptr_;
    return other;
  }

  [[nodiscard]] constexpr auto
  operator==(const any_iterator &other) const -> bool
    requires forward_
  {
    return *iterator_ptr_ == *other.iterator_ptr_;
  }

  constexpr auto operator--() -> any_iterator &
    requires bidirectional_
  {
    --*iterator_ptr_;
    return *this;
  }

  [[nodiscard]] constexpr auto operator--(int) -> any_iterator
    requires bidirectional_
  {
    const auto other = *this;
    --*iterator_ptr_;
    return other;
  }

  [[nodiscard]] constexpr auto
  operator<=>(const any_iterator &other) const -> std::partial_ordering
    requires random_access_
  {
    return *iterator_ptr_ <=> *other.iterator_ptr_;
  }

  [[nodiscard]] constexpr auto
  operator-(const any_iterator &other) const -> difference_type
    requires random_access_
  {
    return *iterator_ptr_ - *other.iterator_ptr_;
  }

  constexpr auto operator+=(difference_type offset) -> any_iterator &
    requires random_access_
  {
    *iterator_ptr_ += offset;
    return *this;
  }

  [[nodiscard]] constexpr auto
  operator+(difference_type offset) const -> any_iterator
    requires random_access_
  {
    auto other = *this;
    other += offset;
    return other;
  }

  [[nodiscard]] constexpr friend auto
  operator+(difference_type offset,
            const any_iterator &iterator) -> any_iterator
    requires random_access_
  {
    return iterator + offset;
  }

  constexpr auto operator-=(difference_type offset) -> any_iterator &
    requires random_access_
  {
    *iterator_ptr_ -= offset;
    return *this;
  }

  [[nodiscard]] constexpr auto
  operator-(difference_type offset) const -> any_iterator
    requires random_access_
  {
    auto other = *this;
    other -= offset;
    return other;
  }

  [[nodiscard]] constexpr auto
  operator[](difference_type offset) const -> reference_
    requires random_access_
  {
    return (*iterator_ptr_)[offset];
  }

  [[nodiscard]] constexpr auto operator->() const -> pointer_
    requires contiguous_
  {
    return std::to_address(*iterator_ptr_);
  }

  [[nodiscard]] constexpr auto
  operator==(std::default_sentinel_t other) const -> bool {
    return *iterator_ptr_ == other;
  }

  [[nodiscard]] constexpr auto type() const noexcept -> const std::type_info & {
    return iterator_ptr_->type();
  }
};

} // namespace pr
