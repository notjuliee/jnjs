#pragma once

#include <quickjs.h>

#include "detail/value_helpers.h"

namespace jnjs {

class borrowed_value;

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
        return detail::value_helpers<T>::is(_ctx, _v) && detail::value_helpers<T>::as(_ctx, _v) == rhs;
    }

    template <typename T> bool kinda_eq(const T &rhs) const {
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
    friend borrowed_value;
    friend detail::value_helpers<value>;
    friend detail::value_helpers<function>;
    friend detail::value_helpers<borrowed_value>;
};

class borrowed_value : public value {
  public:
    borrowed_value() = default;
    ~borrowed_value() noexcept = default;

    borrowed_value(const borrowed_value &o) = default;
    borrowed_value(borrowed_value &&o) noexcept = default;
    borrowed_value &operator=(const borrowed_value &o) noexcept = default;
    borrowed_value &operator=(borrowed_value &&o) noexcept = default;

    [[nodiscard]] value take() const { return value(JS_DupValue(_ctx, _v), _ctx); }

  private:
    explicit borrowed_value(JSValue v, JSContext *ctx) : value(v, ctx) {}
    friend detail::value_helpers<borrowed_value>;
};

namespace detail {
template <> struct value_helpers<value> {
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static value as(JSContext *c, JSValue v) { return value(JS_DupValue(c, v), c); }
    static JSValue from(JSContext *c, const value &v) { return JS_DupValue(c, v._v); }
};
template <> struct value_helpers<borrowed_value> {
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static borrowed_value as(JSContext *c, JSValue v) { return borrowed_value(v, c); }
    static JSValue from(JSContext *c, const borrowed_value &v) { return JS_DupValue(c, v._v); }
};
} // namespace detail

} // namespace jnjs