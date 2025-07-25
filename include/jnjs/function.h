#pragma once
/**
 * @file function.h
 * @brief JS function wrapper.
 */

#include <array>
#include <utility>

#include "detail/function_helpers.h"
#include "value.h"

namespace jnjs {

/**
 * @brief Function wrapper for a JS function.
 */
class function {
  public:
    // Default constructor creates a null function.
    function() = default;

    /**
     * @brief Create a function from a C function.
     * @param ctx JS context to use.
     * @param name Name of the function.
     * @param cfn C function to wrap.
     * @param len Length of the function arguments.
     */
    explicit function(JSContext *ctx, const char *name, JSCFunction *cfn, int len)
        : _v(value(JS_NewCFunction(ctx, cfn, name, len), ctx)) {}

    /**
     * @brief Invoke the function with the given arguments.
     * @tparam Args Type of arguments to pass to function.
     * @param args Arguments to pass to the function.
     * @return Return value of the function.
     */
    template <typename... Args> value operator()(Args... args) {
        const auto ctx = _v._ctx;
        std::array<JSValue, sizeof...(Args)> vargs = {
            detail::arg_list_helpers::set<detail::getter_type_t<Args>>(ctx, args)...};
        return _invoke_impl(vargs.data(), sizeof...(Args));
    }

  private:
    /**
     * @internal
     * @brief Invoke the function with the given JSValue arguments.
     * @param v Argument list.
     * @param c Argument count.
     * @return Return value of the function.
     */
    value _invoke_impl(JSValue *v, int c) const {
        const auto r = JS_Call(_v._ctx, _v._v, _this._v, c, v);
        return value(r, _v._ctx);
    }

    /**
     * @internal
     * @brief Create a function from a JSValue and an optional this value.
     * @param v JSValue representing the function.
     * @param this_ Optional JSValue representing the `this` context for the function.
     */
    explicit function(value v, value this_ = {}) : _v(std::move(v)), _this(std::move(this_)) {}

    value _v = {};    /**< @internal JSValue representing the function. */
    value _this = {}; /**< @internal Optional JSValue representing the `this` context for the function. */
    friend detail::value_helpers<function>;
};

template <> struct detail::value_helpers<function> {
    static bool is(JSContext *c, JSValue v) { return JS_IsFunction(c, v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static function as(JSContext *c, JSValue v) { return function(value(JS_DupValue(c, v), c), {}); }
    static JSValue from(JSContext *c, const function &v) { return JS_DupValue(c, v._v._v); }
}; // namespace detail

} // namespace jnjs