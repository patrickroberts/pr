#pragma once

#include <expected>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PR_MAYBE(...)                                                          \
  ({                                                                           \
    auto &&_result = (__VA_ARGS__);                                            \
    using _type = decltype(_result);                                           \
    if (not _result) {                                                         \
      return ::std::unexpected{static_cast<_type>(_result).error()};           \
    }                                                                          \
    *static_cast<_type>(_result);                                              \
  })
