#pragma once

#include <pr/context_allocator.hpp>
#include <pr/fancy_allocator_adaptor.hpp>
#include <pr/offset_ptr.hpp>

#include <span>
#include <vector>

namespace pr {

template <class T>
using offset_allocator =
    fancy_allocator_adaptor<context_allocator<T>, offset_ptr<T>>;

template <class T>
using vector = std::vector<T, offset_allocator<T>>;

using arg = const char *;

using args = std::span<const arg>;

} // namespace pr
