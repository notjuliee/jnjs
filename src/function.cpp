#include <cassert>
#include <quickjs.h>

#include <jnjs/function.h>

#include "value_helpers.h"

namespace jnjs {

value function::_invoke_impl(detail::qjs::JSValue *v, int c) const {
    auto r = JS_Call(_v._ctx, _v._v, _this._v, c, reinterpret_cast<JSValue *>(v));
    return detail::impl_value_helpers::make_value(_v._ctx, r);
}

} // namespace jnjs