#pragma once

#include <functional>
#include <ranges>

namespace pr {
namespace detail {

// do not assume fancy pointers are constexpr default constructible
template <class Ptr>
inline const Ptr tail = Ptr();

// allow raw pointers to be used in constant expressions
template <class Ptr>
  requires std::is_pointer_v<Ptr>
inline constexpr Ptr tail<Ptr> = Ptr();

template <class T>
struct default_construct {
  constexpr auto operator()() const
      noexcept(std::is_nothrow_default_constructible_v<T>) -> T {
    return T();
  }
};

template <class T>
struct projection {
  T *m_ptr;

  constexpr auto operator()() const noexcept -> T & { return *m_ptr; }
};

} // namespace detail

template <class T, class Allocator = std::allocator<T>>
class forward_list {
  using alloc_traits = std::allocator_traits<Allocator>;

  struct node_type;

  using node_traits = alloc_traits::template rebind_traits<node_type>;
  using node_alloc = node_traits::allocator_type;
  using node_pointer = node_traits::pointer;
  using link_pointer =
      std::pointer_traits<node_pointer>::template rebind<const node_pointer>;
  using link_traits = std::pointer_traits<link_pointer>;

  [[no_unique_address]] node_alloc m_alloc = node_alloc();
  node_pointer m_head = node_pointer();

  static constexpr auto repeat_default(std::size_t count) noexcept {
    using type = detail::default_construct<T>;
    return std::views::repeat(type{}, count) |
           std::views::transform(std::invoke<type>);
  }

  static constexpr auto repeat_copy(std::size_t count,
                                    const T &value) noexcept {
    using type = detail::projection<const T>;
    return std::views::repeat(type{std::addressof(value)}, count) |
           std::views::transform(std::invoke<type>);
  }

  template <class Range>
  constexpr void safe_assign(Range &&range, const Allocator &alloc) {
    forward_list<T, Allocator> temp(alloc);

    auto first = temp.begin();
    auto last = temp.insert_range(first, std::forward<Range>(range));

    clear();
    m_alloc = temp.m_alloc;
    splice(begin(), std::move(first), std::move(last));
  }

public:
  using value_type = T;
  using allocator_type = Allocator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = alloc_traits::pointer;
  using const_pointer = alloc_traits::const_pointer;
  class iterator;
  using const_iterator = std::basic_const_iterator<iterator>;
  using sentinel = std::default_sentinel_t;

  forward_list() = default;

  constexpr explicit forward_list(const Allocator &alloc) noexcept
      : m_alloc(node_alloc(alloc)) {}

  constexpr explicit forward_list(size_type count,
                                  const Allocator &alloc = Allocator())
      : forward_list(std::from_range, repeat_default(count), alloc) {}

  constexpr forward_list(size_type count, const T &value,
                         const Allocator &alloc = Allocator())
      : forward_list(std::from_range, repeat_copy(count, value), alloc) {}

  template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
  constexpr forward_list(Iterator first, Sentinel last,
                         const Allocator &alloc = Allocator())
      : forward_list(std::from_range,
                     std::ranges::subrange{std::move(first), std::move(last)},
                     alloc) {}

  template <std::ranges::input_range Range>
  constexpr forward_list(std::from_range_t tag, Range &&range,
                         const Allocator &alloc = Allocator())
      : forward_list(alloc) {
    prepend_range(std::forward<Range>(range));
  }

  constexpr forward_list(const forward_list &other)
      : forward_list(std::from_range, other,
                     alloc_traits::select_on_container_copy_construction(
                         other.get_allocator())) {}

  constexpr forward_list(forward_list &&other) noexcept
      : m_alloc(other.m_alloc),
        m_head(std::exchange(other.m_head, node_pointer())) {}

  constexpr forward_list(std::initializer_list<T> ilist,
                         const Allocator &alloc = Allocator())
      : forward_list(std::from_range, ilist, alloc) {}

  constexpr ~forward_list() { clear(); }

  constexpr auto operator=(const forward_list &other) -> forward_list & {
    if (this == std::addressof(other)) {
      return *this;
    }

    safe_assign(other,
                alloc_traits::propagate_on_container_copy_assignment::value
                    ? other.get_allocator()
                    : get_allocator());

    return *this;
  }

  constexpr auto operator=(forward_list &&other) noexcept(
      alloc_traits::is_always_equal::value or
      alloc_traits::propagate_on_container_move_assignment::value)
      -> forward_list & {
    if (this == std::addressof(other)) {
      return *this;
    }

    if constexpr (not alloc_traits::propagate_on_container_move_assignment::
                      value) {
      if (m_alloc != other.m_alloc) {
        safe_assign(other | std::views::as_rvalue, get_allocator());
        other.clear();
        return *this;
      }
    }

    std::destroy_at(this);
    std::construct_at(this, std::move(other));

    return *this;
  }

  constexpr void assign(size_type count, const T &value) {
    assign_range(repeat_copy(count, value));
  }

  template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
  constexpr void assign(Iterator first, Sentinel last) {
    assign_range(std::ranges::subrange{std::move(first), std::move(last)});
  }

  constexpr void assign(std::initializer_list<T> ilist) { assign_range(ilist); }

  template <std::ranges::input_range Range>
    requires std::assignable_from<reference,
                                  std::ranges::range_reference_t<Range>>
  constexpr void assign_range(Range &&range) {
    auto &&r = std::forward<Range>(range);
    auto first = std::ranges::begin(r);
    auto last = std::ranges::end(r);
    auto d_first = begin();
    auto d_last = end();

    while (first != last and d_first != d_last) {
      *d_first = *first;
      ++first;
      ++d_first;
    }

    d_first = insert(std::move(d_first), std::move(first), std::move(last));

    erase(std::move(d_first), std::move(d_last));
  }

  [[nodiscard]] constexpr auto get_allocator() const noexcept
      -> allocator_type {
    return allocator_type(m_alloc);
  }

  [[nodiscard]] constexpr auto front() noexcept -> reference {
    return m_head->m_value;
  }

  [[nodiscard]] constexpr auto front() const noexcept -> const_reference {
    return m_head->m_value;
  }

  [[nodiscard]] constexpr auto begin() noexcept -> iterator {
    return iterator{link_traits::pointer_to(m_head)};
  }

  [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {
    return iterator{link_traits::pointer_to(m_head)};
  }

  [[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator {
    return iterator{link_traits::pointer_to(m_head)};
  }

  [[nodiscard]] constexpr auto end() const noexcept -> sentinel {
    return std::default_sentinel;
  }

  [[nodiscard]] constexpr auto cend() const noexcept -> sentinel {
    return std::default_sentinel;
  }

  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return m_head == nullptr;
  }

  [[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
    return node_traits::max_size(m_alloc);
  }

  constexpr void clear() noexcept { erase(begin(), end()); }

  template <class... Args>
  constexpr auto emplace_front(Args &&...args) -> reference {
    return *emplace(begin(), std::forward<Args>(args)...);
  }

  constexpr void push_front(const T &value) { emplace_front(value); }

  constexpr void push_front(T &&value) { emplace_front(std::move(value)); }

  constexpr auto insert(const_iterator pos, const T &value) -> iterator {
    return emplace(std::move(pos), value);
  }

  constexpr auto insert(const_iterator pos, T &&value) -> iterator {
    return emplace(std::move(pos), std::move(value));
  }

  constexpr auto insert(const_iterator pos, size_type count, const T &value)
      -> iterator {
    return insert_range(std::move(pos), repeat_copy(count, value));
  }

  template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
  constexpr auto insert(const_iterator pos, Iterator first, Sentinel last)
      -> iterator {
    return insert_range(std::move(pos), std::ranges::subrange{std::move(first),
                                                              std::move(last)});
  }

  constexpr auto insert(const_iterator pos, std::initializer_list<T> ilist)
      -> iterator {
    return insert_range(std::move(pos), ilist);
  }

  template <class... Args>
  constexpr auto emplace(const_iterator pos, Args &&...args) -> iterator {
    auto new_ptr = node_traits::allocate(m_alloc, 1);

    try {
      node_traits::construct(m_alloc, std::to_address(new_ptr),
                             std::forward<Args>(args)...);
    } catch (...) {
      node_traits::deallocate(m_alloc, std::move(new_ptr), 1);
      throw;
    }

    auto it = std::move(pos).base();
    new_ptr->m_next = std::exchange(it.link(), new_ptr);
    return it;
  }

  constexpr auto erase(const_iterator pos) -> iterator {
    auto it = std::move(pos).base();
    auto old_ptr = std::exchange(it.link(), it.clink()->m_next);

    node_traits::destroy(m_alloc, std::to_address(old_ptr));
    node_traits::deallocate(m_alloc, std::move(old_ptr), 1);

    return it;
  }

  template <std::sentinel_for<const_iterator> Sentinel = sentinel>
  constexpr auto erase(const_iterator first, Sentinel last) -> iterator {
    while (first != last) {
      first = erase(std::move(first));
    }

    return std::move(first).base();
  }

  template <std::ranges::input_range Range>
  constexpr auto insert_range(const_iterator pos, Range &&range) -> iterator {
    forward_list<T, Allocator> temp(get_allocator());
    auto first = temp.begin();
    auto last = first;

    using type = std::ranges::range_reference_t<Range>;

    for (type &&value : std::forward<Range>(range)) {
      last = temp.emplace(std::move(last), std::forward<type>(value));
    }

    splice(pos, std::move(first), std::move(last));

    return std::move(pos).base();
  }

  template <std::ranges::input_range Range>
  constexpr void prepend_range(Range &&range) {
    insert_range(begin(), std::forward<Range>(range));
  }

  constexpr void pop_front() noexcept { erase(begin()); }

  // TODO: resize
  // TODO: swap
  // TODO: merge

  constexpr void splice(const_iterator pos, const_iterator first,
                        const_iterator last) noexcept {
    auto &first_link = first.base().link();
    auto &last_link = last.base().link();
    auto &pos_link = pos.base().link();

    auto old_link = std::exchange(first_link, last_link);
    last_link = std::exchange(pos_link, std::move(old_link));
  }

  // TODO: remove
  // TODO: remove_if
  // TODO: reverse
  // TODO: unique
  // TODO: sort
};

template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel,
          class Alloc = std::allocator<std::iter_value_t<Iterator>>>
forward_list(Iterator, Sentinel, Alloc = Alloc())
    -> forward_list<std::iter_value_t<Iterator>, Alloc>;

template <std::ranges::input_range Range,
          class Alloc = std::allocator<std::ranges::range_value_t<Range>>>
forward_list(std::from_range_t, Range &&, Alloc = Alloc())
    -> forward_list<std::ranges::range_value_t<Range>, Alloc>;

// TODO: operator==
// TODO: operator<=>
// TODO: swap
// TODO: erase
// TODO: erase_if

template <class T, class Allocator>
struct forward_list<T, Allocator>::node_type {
  node_pointer m_next = node_pointer();
  T m_value;

  template <class... Args>
    requires std::constructible_from<T, Args...>
  constexpr node_type(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>)
      : m_value(std::forward<Args>(args)...) {}
};

template <class T, class Allocator>
class forward_list<T, Allocator>::iterator {
  friend forward_list;

  link_pointer m_link_ptr = link_traits::pointer_to(detail::tail<node_pointer>);

  constexpr explicit iterator(link_pointer link_ptr) noexcept
      : m_link_ptr(std::move(link_ptr)) {}

  [[nodiscard]] constexpr auto link() const noexcept -> node_pointer & {
    return const_cast<node_pointer &>(*m_link_ptr);
  }

  [[nodiscard]] constexpr auto clink() const noexcept -> const node_pointer & {
    return *m_link_ptr;
  }

public:
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;
  using iterator_concept = std::forward_iterator_tag;
  using value_type = T;

  constexpr iterator() noexcept = default;

  [[nodiscard]] constexpr auto operator*() const noexcept -> reference {
    return clink()->m_value;
  }

  [[nodiscard]] constexpr auto operator->() const noexcept -> pointer {
    return std::pointer_traits<pointer>::pointer_to(clink()->m_value);
  }

  constexpr auto operator++() noexcept -> iterator & {
    m_link_ptr = link_traits::pointer_to(clink()->m_next);
    return *this;
  }

  [[nodiscard]] constexpr auto operator++(int) noexcept -> iterator {
    auto other = *this;
    ++*this;
    return other;
  }

  [[nodiscard]] constexpr auto operator==(const iterator &other) const noexcept
      -> bool {
    return clink() == other.clink();
  }

  [[nodiscard]] constexpr auto operator==(sentinel other) const noexcept
      -> bool {
    return clink() == nullptr;
  }
};

namespace pmr {

template <class T>
using forward_list = pr::forward_list<T, std::pmr::polymorphic_allocator<T>>;

} // namespace pmr
} // namespace pr
