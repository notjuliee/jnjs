#pragma once

#include "hedley.h"

namespace jnjs {

namespace detail {
struct impl_value_helpers;

template <typename T, typename = void> struct value_helpers {
    HEDLEY_PURE
    HEDLEY_NON_NULL(1)
    static bool is(JSContext *c, JSValue v);
    HEDLEY_PURE
    HEDLEY_NON_NULL(1)
    static bool is_convertible(JSContext *c, JSValue v);
    HEDLEY_NON_NULL(1)
    static T as(JSContext *c, JSValue v);
    HEDLEY_NON_NULL(1)
    static JSValue from(JSContext *c, const T &v);
};
} // namespace detail

template <typename T> struct wrapped_class_builder;

class context;
class function;
class module;
class runtime;
class value;

} // namespace jnjs