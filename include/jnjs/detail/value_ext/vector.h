#pragma once

#include <quickjs.h>

#include <vector>

#include "../fwd.h"

template <typename T> struct jnjs::detail::value_helpers<std::vector<T>> {
    static bool is(JSContext *, JSValue v) { return JS_IsArray(v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static std::vector<T> as(JSContext *c, JSValue v) {
        std::vector<T> ret;
        int64_t len;
        if (JS_GetLength(c, v, &len) < 0 || len < 0) {
            return ret;
        }
        ret.resize(len);
        for (int64_t i = 0; i < len; ++i) {
            auto val = JS_GetPropertyInt64(c, v, i);
            ret.at(i) = std::move(value_helpers<T>::as(c, val));
            JS_FreeValue(c, val);
        }
        return ret;
    }
    static JSValue from(JSContext *c, const std::vector<T> &v) {
        auto rv = JS_NewArray(c);
        for (int64_t i = 0; i < v.size(); ++i) {
            JS_SetPropertyInt64(c, rv, i, value_helpers<T>::from(c, v[i]));
        }
        return rv;
    }
}; // namespace jnjs::detail};