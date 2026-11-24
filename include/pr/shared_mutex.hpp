#pragma once

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <ctime>
#include <limits>
#include <utility>

namespace pr {

class shared_mutex {
  using value_type = std::uint32_t;
  using reference = std::atomic_ref<value_type>;

  static_assert(reference::is_always_lock_free);
  static_assert(reference::required_alignment <= alignof(value_type));

  enum class wait_count_t : value_type {};

  static constexpr value_type mask = std::numeric_limits<value_type>::max();
  static constexpr value_type count_mask = mask >> 1U;
  static constexpr value_type wait_flag = mask & ~count_mask;
  static constexpr value_type max_shared_count = count_mask >> 1U;
  static constexpr value_type exclusive_wait_count = max_shared_count + 1;

  value_type value{count_mask};

  void futex_wait(value_type expected) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    syscall(SYS_futex, std::addressof(value), FUTEX_WAIT, expected,
            static_cast<const timespec *>(nullptr));
  }

  void futex_wake(int waiters) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    syscall(SYS_futex, std::addressof(value), FUTEX_WAKE, waiters);
  }

  // assumes wait_count >= count
  void wait_acquire(wait_count_t wait_count, value_type count) {
    const reference ref{value};
    auto actual = ref.load(std::memory_order_relaxed);

    while (true) {
      // do not acquire count until at least wait_count is available
      while ((actual & count_mask) < std::to_underlying(wait_count)) {
        if ((actual & wait_flag) == 0) {
          // set wait flag
          ref.fetch_or(wait_flag, std::memory_order_relaxed);
          actual |= wait_flag;
        }

        futex_wait(actual);
        actual = ref.load(std::memory_order_relaxed);
      }

      if (ref.compare_exchange_weak(actual, actual - count,
                                    std::memory_order_acquire,
                                    std::memory_order_relaxed)) {
        return;
      }
    }
  }

  auto try_acquire(wait_count_t wait_count, value_type count) -> bool {
    const reference ref{value};
    auto actual = ref.load(std::memory_order_relaxed);

    // ensures forward progress by only trying
    // while at least wait_count are available
    while ((actual & count_mask) >= std::to_underlying(wait_count)) {
      if (ref.compare_exchange_weak(actual, actual - count,
                                    std::memory_order_acquire,
                                    std::memory_order_relaxed)) {
        return true;
      }
    }

    return false;
  }

  void release(value_type count) {
    const reference ref{value};
    auto actual = ref.fetch_add(count, std::memory_order_release);
    actual += count;

    // only notify if wait flag is set and actual count
    // permits at least one waiter to acquire ownership
    if ((actual & wait_flag) == 0 or (actual & count_mask) < max_shared_count) {
      return;
    }

    // clear wait flag
    ref.fetch_and(count_mask, std::memory_order_relaxed);
    futex_wake(std::numeric_limits<int>::max());
  }

public:
  // initializes with no owners
  shared_mutex() = default;
  shared_mutex(const shared_mutex &) = delete;
  shared_mutex(shared_mutex &&) = delete;

  ~shared_mutex() = default;

  auto operator=(const shared_mutex &) -> shared_mutex & = delete;
  auto operator=(shared_mutex &&) -> shared_mutex & = delete;

  // calling from thread that already owns this is UB
  void lock() {
    // phase 1 waits for exclusive owner to release
    wait_acquire(wait_count_t{exclusive_wait_count}, exclusive_wait_count);
    // phase 2 waits for all remaining shared owners to release
    wait_acquire(wait_count_t{max_shared_count}, max_shared_count);
  }

  // calling from thread that already owns this is UB
  void lock_shared() {
    // waits for exclusive owner to release
    // and for exclusive owner acquiring in phase 2
    // and for at least one shared owner to release
    wait_acquire(wait_count_t{exclusive_wait_count + 1}, 1);
  }

  auto try_lock() -> bool {
    return try_acquire(wait_count_t{count_mask}, count_mask);
  }
  auto try_lock_shared() -> bool {
    return try_acquire(wait_count_t{exclusive_wait_count + 1}, 1);
  }

  // calling from thread that does not have exclusive ownership is UB
  void unlock() { release(count_mask); }
  // calling from thread that does not have shared ownership is UB
  void unlock_shared() { release(1); }

  // tries to upgrade shared lock to exclusive without releasing ownership
  // returns true on success
  // calling from thread that does not have shared ownership is UB
  auto try_upgrade_lock() -> bool {
    if (not try_acquire(wait_count_t{exclusive_wait_count},
                        exclusive_wait_count)) {
      return false;
    }
    wait_acquire(wait_count_t{max_shared_count - 1}, max_shared_count - 1);
    return true;
  }

  // downgrades exclusive lock to shared
  // calling from thread that does not have exclusive ownership is UB
  void downgrade_lock() { release(count_mask - 1); }
};

} // namespace pr
