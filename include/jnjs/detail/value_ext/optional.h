#pragma once

#include <quickjs.h>

#include <optional>

#include "../fwd.h"

template <typename T> struct jnjs::detail::value_helpers<std::optional<T>> {
    static bool is(JSContext *c, JSValue v) { return JS_IsNull(v) || JS_IsUndefined(v) || value_helpers<T>::is(c, v); }
    static bool is_convertible(JSContext *c, JSValue v) {
        return JS_IsNull(v) || JS_IsUndefined(v) || value_helpers<T>::is_convertible(c, v);
    }
    static std::optional<T> as(JSContext *c, JSValue v) {
        if (JS_IsNull(v) || JS_IsUndefined(v)) {
            return std::nullopt;
        }
        return value_helpers<T>::as(c, v);
    }
    static JSValue from(JSContext *c, const std::optional<T> &v) {
        if (!v.has_value()) {
            return JS_UNDEFINED;
        }
        return value_helpers<T>::from(c, v.value());
    }
};


