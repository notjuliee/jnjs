#pragma once

#include <quickjs.h>

#include <jnjs/detail/types.h>
#include <jnjs/function.h>

#include <string>

namespace jnjs::detail {

template <typename T> struct value_helpers {
    static bool is(JSContext *c, JSValue v);
    static bool is_convertible(JSContext *c, JSValue v);
    static T as(JSContext *c, JSValue v);
    static JSValue from(JSContext *c, const T &v);
    static constexpr const char *name();
};

struct impl_value_helpers {
    static function make_function(const value &f, const value &t) { return function(f, t); }
    static JSValue get_function_fn(const function &f) { return f._v._v; }
    static value make_value(JSContext *c, JSValue v) { return value(v, c); }
    static JSValue get_v(const value &v) { return v._v; }
    template <typename T> static value make_value_t(JSContext *c, const T &v) {
        return make_value(c, value_helpers<T>::from(c, v));
    }
};

template <> struct value_helpers<undefined> {
    static bool is(JSContext *, JSValue v) { return JS_IsUndefined(v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static undefined as(JSContext *, JSValue) { return {}; }
    static JSValue from(JSContext *, const undefined &) { return JS_UNDEFINED; }
    static constexpr const char *name() { return "undefined"; }
};

template <> struct value_helpers<bool> {
    static bool is(JSContext *, JSValue v) { return JS_IsBool(v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static bool as(JSContext *c, JSValue v) { return JS_ToBool(c, v) != 0; }
    static JSValue from(JSContext *c, const bool &v) { return JS_NewBool(c, v); }
    static constexpr const char *name() { return "bool"; }
};

template <> struct value_helpers<int32_t> {
    static bool is(JSContext *, JSValue v) { return v.tag == JS_TAG_INT; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static int32_t as(JSContext *c, JSValue v) {
        int32_t ret;
        JS_ToInt32(c, &ret, v);
        return ret;
    }
    static JSValue from(JSContext *c, const int32_t &v) { return JS_NewInt32(c, v); }
    static constexpr const char *name() { return "int"; }
};

template <> struct value_helpers<int64_t> {
    static bool is(JSContext *, JSValue v) { return v.tag == JS_TAG_SHORT_BIG_INT || v.tag == JS_TAG_INT; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static int64_t as(JSContext *c, JSValue v) {
        int64_t ret;
        JS_ToInt64(c, &ret, v);
        return ret;
    }
    static JSValue from(JSContext *c, const int64_t &v) { return JS_NewInt64(c, v); }
    static constexpr const char *name() { return "long"; }
};

template <> struct value_helpers<uint32_t> {
    static bool is(JSContext *c, JSValue v) { return value_helpers<int64_t>::is(c, v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static uint32_t as(JSContext *c, JSValue v) { return static_cast<uint32_t>(value_helpers<int64_t>::as(c, v)); }
    static JSValue from(JSContext *c, const uint32_t &v) { return JS_NewInt64(c, v); }
    static constexpr const char *name() { return "uint"; }
};

template <> struct value_helpers<uint64_t> {
    static bool is(JSContext *c, JSValue v) { return value_helpers<int64_t>::is(c, v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static uint32_t as(JSContext *c, JSValue v) { return static_cast<uint64_t>(value_helpers<int64_t>::as(c, v)); }
    static JSValue from(JSContext *c, const uint64_t &v) { return JS_NewInt64(c, static_cast<const int64_t &>(v)); }
    static constexpr const char *name() { return "ulong"; }
};

template <> struct value_helpers<std::string> {
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
    static constexpr const char *name() { return "string"; }
};

template <> struct value_helpers<function> {
    static bool is(JSContext *c, JSValue v) { return JS_IsFunction(c, v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static function as(JSContext *c, JSValue v) {
        return impl_value_helpers::make_function(impl_value_helpers::make_value(c, JS_DupValue(c, v)), {});
    }
    static JSValue from(JSContext *c, const function &v) {
        return JS_DupValue(c, impl_value_helpers::get_function_fn(v));
    }
    static constexpr const char *name() { return "function"; }
};

template <> struct value_helpers<value> {
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static value as(JSContext *c, JSValue v) { return impl_value_helpers::make_value(c, v); }
    static JSValue from(JSContext *, const value &v) { return impl_value_helpers::get_v(v); }
    static constexpr const char *name() { return "any"; }
};

template <typename T> struct value_helpers<must_be<T>> {
    static bool is(JSContext *c, JSValue v) { return value_helpers<T>::is(c, v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static T as(JSContext *c, JSValue v) { return value_helpers<T>::as(c, v); }
    static JSValue from(JSContext *c, const T &v) { return value_helpers<T>::from(c, v); }
    static constexpr const char *name() { return value_helpers<T>::name(); }
};

} // namespace jnjs::detail
