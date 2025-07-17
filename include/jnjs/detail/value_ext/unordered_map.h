#pragma once

#include <quickjs.h>

#include <unordered_map>

#include "../fwd.h"

template <typename Tk, typename Tv> struct jnjs::detail::value_helpers<std::unordered_map<Tk, Tv>> {
    static bool is(JSContext *, JSValue v) { return JS_IsObject(v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static std::unordered_map<Tk, Tv> as(JSContext *c, JSValue v) {
        std::unordered_map<Tk, Tv> ret;
        JSPropertyEnum *tab;
        uint32_t len;
        if (JS_GetOwnPropertyNames(c, &tab, &len, v, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK) < 0) {
            return ret;
        }
        for (uint32_t i = 0; i < len; ++i) {
            auto kv = JS_AtomToValue(c, tab[i].atom);
            auto key = value_helpers<Tk>::as(c, kv);
            JS_FreeValue(c, kv);
            auto val = JS_GetProperty(c, v, tab[i].atom);
            ret.emplace(std::move(key), value_helpers<Tv>::as(c, val));
            JS_FreeValue(c, val);
        }
        js_free(c, tab);
        return ret;
    }
    static JSValue from(JSContext *c, const std::unordered_map<Tk, Tv> &vm) {
        auto rv = JS_NewObject(c);
        for (const auto &[k, v] : vm) {
            auto jk = value_helpers<Tk>::from(c, k);
            auto atom = JS_ValueToAtom(c, jk);
            JS_DefinePropertyValue(c, rv, atom, value_helpers<Tv>::from(c, v), JS_PROP_C_W_E);
            JS_FreeAtom(c, atom);
        }
        return rv;
    }
}; // namespace jnjs::detail