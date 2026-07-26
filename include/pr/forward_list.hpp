#pragma once

#include <functional>
#include <ranges>

namespace pr {
namespace detail {

template <class T, class Allocator>
struct node {
  using node_traits =
      std::allocator_traits<Allocator>::template rebind_traits<node>;
  using node_pointer = node_traits::pointer;

  node_pointer m_next = node_pointer();
  T m_value;

  template <class... Args>
    requires std::constructible_from<T, Args...>
  constexpr node(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>)
      : m_value(std::forward<Args>(args)...) {}
};

// do not assume fancy pointers are constexpr default constructible
template <class Ptr>
inline const Ptr tail = Ptr();

// allow raw pointers to be used in constant expressions
template <class Ptr>
  requires std::is_pointer_v<Ptr>
inline constexpr Ptr tail<Ptr> = Ptr();

template <class T>
struct default_generator {
  constexpr auto operator()() const
      noexcept(std::is_nothrow_default_constructible_v<T>) -> T {
    return T();
  }
};

template <class T>
struct copy_generator {
  T *m_ptr;

  constexpr auto operator()() const noexcept -> T & { return *m_ptr; }
};

template <class Generator>
constexpr auto repeat(std::size_t count, Generator generator) noexcept {
  return std::views::repeat(std::move(generator), count) |
         std::views::transform(
             [](const Generator &generator) noexcept(
                 std::is_nothrow_invocable_v<const Generator &>)
                 -> std::invoke_result_t<const Generator &> {
               return generator();
             });
}

template <class T>
constexpr auto repeat_default(std::size_t count) noexcept {
  return repeat(count, default_generator<T>{});
}

template <class T>
constexpr auto repeat_copy(std::size_t count, const T &value) noexcept {
  return repeat(count, copy_generator{std::addressof(value)});
}

template <class T>
struct synth_three_way_result {
  using type = std::weak_ordering;
};

template <std::three_way_comparable T>
struct synth_three_way_result<T> {
  using type = std::compare_three_way_result_t<T>;
};

template <class T>
using synth_three_way_result_t = synth_three_way_result<T>::type;

} // namespace detail

template <class T, class Allocator = std::allocator<T>>
class forward_list {
  using alloc_traits = std::allocator_traits<Allocator>;
  using node_type = detail::node<T, Allocator>;
  using node_traits = node_type::node_traits;
  using node_alloc = node_traits::allocator_type;
  using node_pointer = node_traits::pointer;
  using link_pointer =
      std::pointer_traits<node_pointer>::template rebind<const node_pointer>;
  using link_traits = std::pointer_traits<link_pointer>;

  [[no_unique_address]] node_alloc m_alloc = node_alloc();
  node_pointer m_head = node_pointer();

  template <bool Propagate, class Range>
  constexpr void safe_assign(Range &&range, const Allocator &alloc) {
    forward_list<T, Allocator> temp(alloc);

    auto first = temp.begin();
    auto last = temp.insert_range(first, std::forward<Range>(range));

    clear();
    if constexpr (Propagate) {
      m_alloc = temp.m_alloc;
    }
    splice(begin(), std::move(first), std::move(last));
  }

  template <class Generator>
  constexpr void resize_impl(std::size_t count, Generator generator) {
    auto first = begin();
    const auto last = end();
    const auto size =
        static_cast<size_type>(std::ranges::advance(first, count, last));

    if (size >= count) {
      erase(std::move(first), last);
    } else {
      insert_range(std::move(first),
                   detail::repeat(count - size, std::move(generator)));
    }
  }

  constexpr void splice_impl(node_pointer &pos, node_pointer &first,
                             node_pointer &last) noexcept {
    auto old = std::exchange(first, last);
    last = std::exchange(pos, std::move(old));
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
      : forward_list(std::from_range, detail::repeat_default<T>(count), alloc) {
  }

  constexpr forward_list(size_type count, const T &value,
                         const Allocator &alloc = Allocator())
      : forward_list(std::from_range, detail::repeat_copy(count, value),
                     alloc) {}

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

    constexpr auto pocca =
        alloc_traits::propagate_on_container_copy_assignment::value;

    safe_assign<pocca>(other, pocca ? other.get_allocator() : get_allocator());

    return *this;
  }

  constexpr auto operator=(forward_list &&other) noexcept(
      alloc_traits::is_always_equal::value or
      alloc_traits::propagate_on_container_move_assignment::value)
      -> forward_list & {
    if (this == std::addressof(other)) {
      return *this;
    }

    constexpr auto pocma =
        alloc_traits::propagate_on_container_move_assignment::value;

    if constexpr (not pocma) {
      if (m_alloc != other.m_alloc) {
        safe_assign<pocma>(other | std::views::as_rvalue, get_allocator());
        other.clear();
        return *this;
      }
    }

    std::destroy_at(this);
    std::construct_at(this, std::move(other));

    return *this;
  }

  constexpr void assign(size_type count, const T &value) {
    assign_range(detail::repeat_copy(count, value));
  }

  template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
  constexpr void assign(Iterator first, Sentinel last) {
    assign_range(std::ranges::subrange{std::move(first), std::move(last)});
  }

  constexpr void assign(std::initializer_list<T> ilist) { assign_range(ilist); }

  template <std::ranges::input_range Range>
    requires std::assignable_from<reference,
                                  std::ranges::range_reference_t<Range>>
  // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
  constexpr void assign_range(Range &&range) {
    auto first = std::ranges::begin(range);
    auto last = std::ranges::end(range);
    auto d_first = begin();
    auto d_last = end();

    while (first != last and d_first != d_last) {
      *d_first = *first;
      ++first;
      ++d_first;
    }

    d_first = insert(std::move(d_first), std::move(first), std::move(last));

    erase(std::move(d_first), d_last);
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
    return insert_range(std::move(pos), detail::repeat_copy(count, value));
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

    using reference_type = std::ranges::range_reference_t<Range>;

    for (reference_type &&reference : std::forward<Range>(range)) {
      last = temp.emplace(std::move(last),
                          std::forward<reference_type>(reference));
      ++last;
    }

    splice(pos, std::move(first), std::move(last));

    return std::move(pos).base();
  }

  template <std::ranges::input_range Range>
  constexpr void prepend_range(Range &&range) {
    insert_range(begin(), std::forward<Range>(range));
  }

  constexpr void pop_front() noexcept { erase(begin()); }

  constexpr void resize(size_type count) {
    resize_impl(count, detail::default_generator<T>{});
  }

  constexpr void resize(size_type count, const T &value) {
    resize_impl(count, detail::copy_generator{std::addressof(value)});
  }

  constexpr void
  swap(forward_list &other) noexcept(alloc_traits::is_always_equal::value) {
    using std::swap;

    if constexpr (alloc_traits::propagate_on_container_swap::value) {
      swap(m_alloc, other.m_alloc);
    }

    swap(m_head, other.m_head);
  }

  constexpr void merge(forward_list &other) { merge(std::move(other)); }

  constexpr void merge(forward_list &&other) {
    merge(std::move(other), std::less{});
  }

  template <std::predicate<const T &, const T &> Compare>
  constexpr void merge(forward_list &other, Compare comp) {
    merge(std::move(other), std::move(comp));
  }

  template <std::predicate<const T &, const T &> Compare>
  // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
  constexpr void merge(forward_list &&other, Compare comp) {
    if (this == std::addressof(other)) {
      return;
    }

    auto d_first = cbegin();
    auto d_last = cend();
    auto first = other.cbegin();
    auto last = other.cend();

    while (d_first != d_last and first != last) {
      if (comp(*first, *d_first)) {
        splice(d_first, first);
      } else {
        ++d_first;
      }
    }

    if (first != last) {
      d_first.base().link() =
          std::exchange(first.base().link(), node_pointer());
    }
  }

  constexpr void splice(const_iterator pos, const_iterator first,
                        const_iterator last) noexcept {
    splice_impl(pos.base().link(), first.base().link(), last.base().link());
  }

  constexpr void splice(const_iterator pos, const_iterator it) noexcept {
    auto &first = it.base().link();
    splice_impl(pos.base().link(), first, first->m_next);
  }

  constexpr auto remove(const T &value) -> size_type {
    return remove_if(
        [&](const T &element) -> bool { return element == value; });
  }

  template <std::predicate<const T &> UnaryPredicate>
  constexpr auto remove_if(UnaryPredicate p) -> size_type {
    size_type result = 0;
    forward_list<T, Allocator> temp(get_allocator());
    auto first = cbegin();
    const auto last = cend();

    while (first != last) {
      if (p(*first)) {
        temp.splice(temp.cbegin(), first);
        ++result;
      } else {
        ++first;
      }
    }

    return result;
  }

  constexpr void reverse() noexcept {
    const auto first = begin();
    const auto last = end();

    if (auto it = first; it != last) {
      ++it;

      while (it != last) {
        splice(first, it);
      }
    }
  }

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

template <class T, class Allocator>
constexpr auto operator==(const forward_list<T, Allocator> &lhs,
                          const forward_list<T, Allocator> &rhs) -> bool {
  return std::ranges::equal(lhs, rhs);
}

template <class T, class Allocator>
constexpr auto operator<=>(const forward_list<T, Allocator> &lhs,
                           const forward_list<T, Allocator> &rhs)
    -> detail::synth_three_way_result_t<T> {
  if constexpr (std::three_way_comparable<T>) {
    return std::lexicographical_compare_three_way(lhs.begin(), {}, rhs.begin(),
                                                  {});
  } else {
    return std::lexicographical_compare_three_way(
        lhs.begin(), {}, rhs.begin(), {}, std::compare_weak_order_fallback);
  }
}

template <class T, class Allocator>
constexpr void
swap(forward_list<T, Allocator> &lhs,
     forward_list<T, Allocator> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

template <class T, class Allocator, class U = T>
constexpr auto erase(forward_list<T, Allocator> &c, const U &value)
    -> forward_list<T, Allocator>::size_type {
  return c.remove_if(
      [&](const T &element) -> bool { return element == value; });
}

template <class T, class Allocator, std::predicate<const T &> Pred>
constexpr auto erase_if(forward_list<T, Allocator> &c, Pred pred)
    -> forward_list<T, Allocator>::size_type {
  return c.remove_if(std::move(pred));
}

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

template <class T, class Allocator, class OtherAllocator>
struct std::uses_allocator<pr::detail::node<T, Allocator>, OtherAllocator>
    : std::uses_allocator<T, OtherAllocator> {};
