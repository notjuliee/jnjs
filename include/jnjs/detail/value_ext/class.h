#pragma once

#include <quickjs.h>

#include "../fwd.h"
#include "../js_storage.h"
#include "../type_traits.h"
#include "jnjs/binding.h"

template <typename T>
struct jnjs::detail::value_helpers<jnjs::detail::stored_class<T> *, std::enable_if_t<jnjs::detail::has_build_v<T>>> {
    static bool is(JSContext *c, JSValue v) {
        return JS_GetClassID(v) == internal_class_meta<stored_class<T>>::data.id;
    }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static stored_class<T> *as(JSContext *c, JSValue v) {
        return static_cast<stored_class<T> *>(JS_GetOpaque(v, internal_class_meta<stored_class<T>>::data.id));
    }
    static JSValue from(JSContext *c, stored_class<T> *v) {
        auto o = JS_NewObjectClass(c, internal_class_meta<stored_class<T>>::data.id);
        JS_SetOpaque(o, v);
        return o;
    }
};

// Borrowed classes
template <typename T> struct jnjs::detail::value_helpers<T *, std::enable_if_t<jnjs::detail::has_build_v<T>>> {
    using Ts = stored_class<T>;
    static bool is(JSContext *, JSValue v) { return JS_GetClassID(v) == internal_class_meta<Ts>::data.id; }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static T *as(JSContext *, JSValue v) {
        auto *holder = static_cast<Ts *>(JS_GetOpaque(v, internal_class_meta<Ts>::data.id));
        if (!holder)
            return nullptr;
        return holder->get();
    }
    static JSValue from(JSContext *c, T *v) {
        auto o = JS_NewObjectClass(c, internal_class_meta<Ts>::data.id);
        JS_SetOpaque(o, new borrowed_stored_class<T>(v));
        return o;
    }
};

// Owned classes
template <typename T> struct jnjs::detail::value_helpers<T, std::enable_if_t<jnjs::detail::has_build_v<T>>> {
    using Ts = stored_class<T>;
    static bool is(JSContext *, JSValue v) { return JS_GetClassID(v) == internal_class_meta<Ts>::data.id; }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static T as(JSContext *, JSValue v) {
        auto *holder = static_cast<Ts *>(JS_GetOpaque(v, internal_class_meta<Ts>::data.id));
        return *holder->get();
    }
    static JSValue from(JSContext *c, const T &v) {
        auto o = JS_NewObjectClass(c, internal_class_meta<Ts>::data.id);
        JS_SetOpaque(o, new owned_stored_class<T>(v));
        return o;
    }
};