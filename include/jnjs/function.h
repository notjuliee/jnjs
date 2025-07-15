#pragma once

#include <array>
#include <utility>

#include "detail/function_helpers.h"
#include "value.h"

namespace jnjs {

class function {
  public:
    function() = default;

    template <typename... Args> value operator()(Args... args) {
        const auto ctx = _v._ctx;
        std::array<detail::qjs::JSValue, sizeof...(Args)> vargs = {
            detail::arg_list_helpers::set<detail::getter_type_t<Args>>(ctx, args)...};
        return _invoke_impl(vargs.data(), sizeof...(Args));
    }

  private:
    value _invoke_impl(detail::qjs::JSValue *v, int c) const;

    explicit function(value v, value this_ = {}) : _v(std::move(v)), _this(std::move(this_)) {}

    value _v;
    value _this;
    friend detail::impl_value_helpers;
};

} // namespace jnjs