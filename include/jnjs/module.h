#pragma once

#include <quickjs.h>
#include <vector>

#include "binding.h"

#include "detail/type_traits.h"

namespace jnjs {

namespace detail {
struct module_builder_class {
    class_builder_data d;
    internal_class_meta_data *id = nullptr;
};
struct module_builder_data {
    constexpr static uint32_t max_function_count = 256;
    constexpr static uint32_t max_class_count = 64;
    const char *name = nullptr;
    JSCFunctionListEntry functions[max_function_count] = {};
    module_builder_class classes[max_class_count] = {};
    size_t function_count = 0;
    size_t class_count = 0;
};

} // namespace detail

} // namespace jnjs
