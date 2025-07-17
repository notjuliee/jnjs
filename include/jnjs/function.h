#pragma once

#include <array>
#include <utility>

#include "detail/function_helpers.h"
#include "value.h"

namespace jnjs {

class function {
  public:
    function() = default;
    explicit function(JSContext *ctx, const char *name, JSCFunction *cfn, int len)
        : _v(value(JS_NewCFunction(ctx, cfn, name, len), ctx)) {}

    template <typename... Args> value operator()(Args... args) {
        const auto ctx = _v._ctx;
        std::array<JSValue, sizeof...(Args)> vargs = {
            detail::arg_list_helpers::set<detail::getter_type_t<Args>>(ctx, args)...};
        return _invoke_impl(vargs.data(), sizeof...(Args));
    }

  private:
    value _invoke_impl(JSValue *v, int c) const {
        auto r = JS_Call(_v._ctx, _v._v, _this._v, c, v);
        return value(r, _v._ctx);
    }

    explicit function(value v, value this_ = {}) : _v(std::move(v)), _this(std::move(this_)) {}

    value _v = {};
    value _this = {};
    friend detail::value_helpers<function>;
};

namespace detail {

template <> struct value_helpers<function> {
    static bool is(JSContext *c, JSValue v) { return JS_IsFunction(c, v); }
    static bool is_convertible(JSContext *c, JSValue v) { return is(c, v); }
    static function as(JSContext *c, JSValue v) { return function(value(JS_DupValue(c, v), c), {}); }
    static JSValue from(JSContext *c, const function &v) { return JS_DupValue(c, v._v._v); }
};

} // namespace detail

} // namespace jnjs