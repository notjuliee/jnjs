#pragma once

#include <quickjs.h>
#include <string>

#include "../fwd.h"

template <> struct jnjs::detail::value_helpers<std::string> {
    static bool is(JSContext *, JSValue v) { return JS_IsString(v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static std::string as(JSContext *c, JSValue v) {
        auto s = JS_ToCString(c, v);
        if (s == nullptr && JS_IsException(v)) {
            auto e = JS_GetException(c);
            s = JS_ToCString(c, e);
            JS_FreeValue(c, e);
        }
        std::string ret(s ? s : "<error>");
        if (s)
            JS_FreeCString(c, s);
        return ret;
    }
    static JSValue from(JSContext *c, const std::string &v) { return JS_NewString(c, v.c_str()); }
};

