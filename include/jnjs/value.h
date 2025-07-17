#pragma once

#include <quickjs.h>

#include "detail/value_helpers.h"

namespace jnjs {

class value {
  public:
    value() = default;
    ~value() noexcept {
        if (_ctx != nullptr) {
            JS_FreeValue(_ctx, _v);
        }
    }

    value(const value &o) { *this = o; }
    value(value &&o) noexcept { *this = std::forward<value>(o); }
    value &operator=(const value &o) noexcept {
        if (this != &o) {
            if (o._ctx != nullptr) {
                _v = JS_DupValue(o._ctx, o._v);
                _ctx = o._ctx;
            } else {
                _v = o._v;
                _ctx = nullptr;
            }
        }
        return *this;
    };
    value &operator=(value &&o) noexcept {
        if (this != &o) {
            _v = o._v;
            _ctx = o._ctx;
            o._v = JS_UNDEFINED;
            o._ctx = nullptr;
        }
        return *this;
    }

    template <typename T> bool operator==(const T &rhs) const {
        return detail::value_helpers<T>::is_convertible(_ctx, _v) && detail::value_helpers<T>::as(_ctx, _v) == rhs;
    }

    template <typename T> bool is() const { return detail::value_helpers<T>::is(_ctx, _v); }
    template <typename T> bool is_convertible() const { return detail::value_helpers<T>::is_convertible(_ctx, _v); }
    template <typename T> T as() const { return detail::value_helpers<T>::as(_ctx, _v); }

  private:
    explicit value(JSValue v, JSContext *ctx) : _v(v), _ctx(ctx) {}
    JSValue _v = JS_UNDEFINED;
    JSContext *_ctx = nullptr;
    friend context;
    friend function;
    friend detail::value_helpers<value>;
    friend detail::value_helpers<function>;
};

namespace detail {
template <> struct value_helpers<value> {
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static value as(JSContext *c, JSValue v) { return value(JS_DupValue(c, v), c); }
    static JSValue from(JSContext *c, const value &v) { return JS_DupValue(c, v._v); }
};
} // namespace detail

} // namespace jnjs