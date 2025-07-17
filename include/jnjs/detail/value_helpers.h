#pragma once

#include <quickjs.h>

#include <string>

#include "fwd.h"
#include "hedley.h"
#include "type_traits.h"
#include "types.h"

namespace jnjs::detail {

template <> struct value_helpers<undefined> {
    static bool is(JSContext *, JSValue v) { return JS_IsUndefined(v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static undefined as(JSContext *, JSValue) { return {}; }
    static JSValue from(JSContext *, const undefined &) { return JS_UNDEFINED; }
};

template <> struct value_helpers<bool> {
    static bool is(JSContext *, JSValue v) { return JS_IsBool(v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static bool as(JSContext *c, JSValue v) { return JS_ToBool(c, v) != 0; }
    static JSValue from(JSContext *c, const bool &v) { return JS_NewBool(c, v); }
};

template <> struct value_helpers<int32_t> {
    HEDLEY_CONST
    constexpr static bool is(JSContext *, JSValue v) { return v.tag == JS_TAG_INT; }
    HEDLEY_CONST
    constexpr static bool is_convertible(JSContext *, JSValue) { return true; }
    HEDLEY_NON_NULL(1)
    HEDLEY_CONST
    static int32_t as(JSContext *c, JSValue v) {
        int32_t ret;
        JS_ToInt32(c, &ret, v);
        return ret;
    }
    HEDLEY_CONST
    constexpr static JSValue from(JSContext *, const int32_t &v) { return JSValue{v, JS_TAG_INT}; }
};

template <> struct value_helpers<int64_t> {
    HEDLEY_NON_NULL(1)
    HEDLEY_CONST
    constexpr static bool is(JSContext *, JSValue v) { return v.tag == JS_TAG_SHORT_BIG_INT || v.tag == JS_TAG_INT; }
    HEDLEY_NON_NULL(1)
    HEDLEY_CONST
    constexpr static bool is_convertible(JSContext *, JSValue) { return true; }
    HEDLEY_NON_NULL(1)
    static int64_t as(JSContext *c, JSValue v) {
        int64_t ret;
        JS_ToInt64(c, &ret, v);
        return ret;
    }
    HEDLEY_NON_NULL(1)
    static JSValue from(JSContext *c, const int64_t &v) { return JS_NewInt64(c, v); }
};

template <> struct value_helpers<uint32_t> {
    static bool is(JSContext *c, JSValue v) { return value_helpers<int64_t>::is(c, v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static uint32_t as(JSContext *c, JSValue v) { return static_cast<uint32_t>(value_helpers<int64_t>::as(c, v)); }
    static JSValue from(JSContext *c, const uint32_t &v) { return JS_NewInt64(c, v); }
};

template <> struct value_helpers<uint64_t> {
    static bool is(JSContext *c, JSValue v) { return value_helpers<int64_t>::is(c, v); }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static uint32_t as(JSContext *c, JSValue v) { return static_cast<uint64_t>(value_helpers<int64_t>::as(c, v)); }
    static JSValue from(JSContext *c, const uint64_t &v) { return JS_NewInt64(c, static_cast<const int64_t &>(v)); }
};

template <> struct value_helpers<JSValue> { // lol
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static JSValue as(JSContext *c, JSValue v) { return JS_DupValue(c, v); }
    static JSValue from(JSContext *c, const JSValue &v) { return JS_DupValue(c, v); }
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
};

template <typename T> struct value_helpers<T *, std::enable_if_t<has_build_v<T>>> {
    static bool is(JSContext *c, JSValue v) { return JS_GetClassID(v) == internal_class_meta<T>::data.id; }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static T *as(JSContext *c, JSValue v) {
        return reinterpret_cast<T *>(JS_GetOpaque(v, internal_class_meta<T>::data.id));
    }
    static JSValue from(JSContext *c, T *v) {
        auto ret = JS_NewObjectClass(c, internal_class_meta<T>::data.id);
        JS_SetOpaque(ret, v);
        return ret;
    }
};

template <typename T> struct value_helpers<must_be<T>> {
    static bool is(JSContext *c, JSValue v) { return value_helpers<T>::is(c, v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static T as(JSContext *c, JSValue v) { return value_helpers<T>::as(c, v); }
    static JSValue from(JSContext *c, const T &v) { return value_helpers<T>::from(c, v); }
};

template <typename T> struct value_helpers<std::optional<T>> {
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

} // namespace jnjs::detail
