#pragma once

#include <cstddef>

#ifndef JNJS_WRAPPED_CLASS_MAX_FUNCTIONS
/// Max number of functions that can be bound on a wrapped class
#define JNJS_WRAPPED_CLASS_MAX_FUNCTIONS 128
#endif

namespace jnjs::config {

inline constexpr std::size_t wrapped_class_max_functions = JNJS_WRAPPED_CLASS_MAX_FUNCTIONS;

} // namespace jnjs::config
