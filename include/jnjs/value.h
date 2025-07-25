#pragma once
/**
 * @file value.h
 * @brief Wrapper classes for JavaScript values.
 */

#include <quickjs.h>

#include "detail/value_helpers.h"

namespace jnjs {

class borrowed_value;

/**
 * @brief Wrapper class for JavaScript values.
 */
class value {
  public:
    // Create a default value, which is `undefined`.
    value() = default;
    // Release the value
    ~value() noexcept {
        if (_ctx != nullptr) {
            JS_FreeValue(_ctx, _v);
        }
    }

    // Create a copy of the value
    value(const value &o) { *this = o; }
    // Move the value, transferring ownership
    value(value &&o) noexcept { *this = std::forward<value>(o); }
    // Move the value, transferring ownership
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
    }
    // Create a copy of the value
    value &operator=(value &&o) noexcept {
        if (this != &o) {
            _v = o._v;
            _ctx = o._ctx;
            o._v = JS_UNDEFINED;
            o._ctx = nullptr;
        }
        return *this;
    }

    /**
     * @brief Get a property of the value by name.
     * @param name Name of the property to access.
     * @return The value of the property, or undefined if the property does not exist.
     */
    value operator[](const char *name) const noexcept {
        if (HEDLEY_UNLIKELY(_ctx == nullptr)) {
            return {};
        }
        return value(JS_GetPropertyStr(_ctx, _v, name), _ctx);
    }

    /**
     * @brief Get a property of the value by index.
     * @param idx Index of the property to access.
     * @return The value of the property at the specified index, or undefined if the index is out of bounds.
     */
    value operator[](const int idx) const noexcept {
        if (HEDLEY_UNLIKELY(_ctx == nullptr)) {
            return {};
        }
        return value(JS_GetPropertyInt64(_ctx, _v, idx), _ctx);
    }

    /**
     * @brief Get a property of the value by another value.
     * @param other Value of the property key to access.
     * @return The value of the property, or undefined if the property does not exist.
     */
    value operator[](const value &other) const noexcept {
        if (HEDLEY_UNLIKELY(_ctx == nullptr)) {
            return {};
        }
        const auto a = JS_ValueToAtom(_ctx, other._v);
        const auto r = JS_GetProperty(_ctx, _v, a);
        JS_FreeAtom(_ctx, a);
        return value(r, _ctx);
    }

    /**
     * @brief Strictly compare this value to another value.
     * @tparam T Type to compare to
     * @param rhs Value to compare against.
     * @return If the values are both of the same type and equal.
     */
    template <typename T> bool operator==(const T &rhs) const {
        return detail::value_helpers<T>::is(_ctx, _v) && detail::value_helpers<T>::as(_ctx, _v) == rhs;
    }

    /**
     * @brief Loosely compare this value to another value.
     * @tparam T Type to compare to
     * @param rhs Value to compare against.
     * @return If the values are convertible to the same type and equal.
     */
    template <typename T> bool kinda_eq(const T &rhs) const {
        return detail::value_helpers<T>::is_convertible(_ctx, _v) && detail::value_helpers<T>::as(_ctx, _v) == rhs;
    }

    /**
     * @brief Check if the value is of a specific type.
     * @tparam T Type to check against.
     * @return If the value is of type T.
     */
    template <typename T> [[nodiscard]] bool is() const { return detail::value_helpers<T>::is(_ctx, _v); }
    /**
     * @brief Check if the value is convertible to a specific type.
     * @tparam T Type to check against.
     * @return If the value is convertible to type T.
     */
    template <typename T> [[nodiscard]] bool is_convertible() const {
        return detail::value_helpers<T>::is_convertible(_ctx, _v);
    }
    /**
     * @brief Convert the value to a specific type.
     * @tparam T Type to convert to.
     * @return The value converted to type T.
     * @throws detail::js_exception if the value is not convertible to type T.
     */
    template <typename T> T as() const { return detail::value_helpers<T>::as(_ctx, _v); }

  private:
    /**
     * @internal Create a value from a JSValue and a JSContext.
     * @param v JSValue to wrap.
     * @param ctx JavaScript context in which the value exists.
     */
    explicit value(JSValue v, JSContext *ctx) : _v(v), _ctx(ctx) {}

    JSValue _v = JS_UNDEFINED; /**< @internal The underlying JSValue. */
    JSContext *_ctx = nullptr; /**< @internal The JavaScript context in which the value exists. */
    friend context;
    friend function;
    friend detail::value_helpers<value>;
    friend detail::value_helpers<function>;
};

template <> struct detail::value_helpers<value> {
    static bool is(JSContext *, JSValue) { return true; }
    static bool is_convertible(JSContext *, JSValue) { return true; }
    static value as(JSContext *c, JSValue v) { return value(JS_DupValue(c, v), c); }
    static JSValue from(JSContext *c, const value &v) { return JS_DupValue(c, v._v); }
}; // namespace detail

} // namespace jnjs