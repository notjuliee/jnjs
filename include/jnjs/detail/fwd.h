#pragma once

namespace jnjs {

namespace detail {
struct impl_value_helpers;

template <typename T, typename = void> struct value_helpers {
    static bool is(JSContext *c, JSValue v);
    static bool is_convertible(JSContext *c, JSValue v);
    static T as(JSContext *c, JSValue v);
    static JSValue from(JSContext *c, const T &v);
};
} // namespace detail

template <typename T> struct wrapped_class_builder;

class context;
class function;
class runtime;
class value;

} // namespace jnjs