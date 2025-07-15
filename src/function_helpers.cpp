#include <quickjs.h>

#include <jnjs/detail/function_helpers.h>

#include <jnjs/value.h>

#include "value_helpers.h"

namespace jnjs::detail {

HEDLEY_NON_NULL(2)
bool arg_list_helpers::is_null_or_undefined(int argc, qjs::JSValue *argv, int i) {
    return i >= argc || JS_IsNull(argv[i]) || JS_IsUndefined(argv[i]);
}

HEDLEY_NON_NULL(1, 2)
HEDLEY_NO_RETURN
void arg_list_helpers::asserters::assertion_failure(void *HEDLEY_RESTRICT ctx, const char *HEDLEY_RESTRICT msg) {
    throw js_exception(JS_ThrowTypeError(static_cast<JSContext *>(ctx), "%s", msg));
}

template <typename T>
HEDLEY_NON_NULL(1, 3)
HEDLEY_ALWAYS_INLINE HEDLEY_PURE std::decay_t<T> pull_value_impl(
    JSContext *c, int argc, qjs::JSValue *argv, int i, bool strict) {
    using H = value_helpers<std::decay_t<T>>;
    if (HEDLEY_UNLIKELY(i >= argc)) {
        throw js_exception(JS_ThrowRangeError(c, "Argument out of range (%d >= %d)", i, argc));
    }
    if (!(strict ? H::is(c, argv[i]) : H::is_convertible(c, argv[i])))
        throw js_exception(JS_ThrowTypeError(c, "Argument %d is not of type %s", i, H::name()));
    return H::as(c, argv[i]);
}
template <typename T> HEDLEY_NON_NULL(1) HEDLEY_ALWAYS_INLINE JSValue push_value_impl(JSContext *c, const T &v) {
    using H = value_helpers<std::decay_t<T>>;
    return H::from(c, v);
}

template <>
impl_wrapped_class pull_value_impl<impl_wrapped_class>(JSContext *c, int argc, qjs::JSValue *argv, int i, bool strict) {
    // We abuse the strict parameter to allow optional references
    impl_wrapped_class ret = {nullptr, 0};
    if (!strict && arg_list_helpers::is_null_or_undefined(argc, argv, i)) {
        return ret;
    }
    if (HEDLEY_UNLIKELY(i >= argc)) {
        throw js_exception(JS_ThrowRangeError(c, "Argument out of range (%d >= %d)", i, argc));
    }
    ret.id = JS_GetClassID(argv[i]);
    if (HEDLEY_UNLIKELY(ret.id == 0)) {
        throw js_exception(JS_ThrowTypeError(c, "Argument %d is not an object", i));
    }
    ret.ptr = JS_GetOpaque2(c, argv[i], ret.id);
    if (HEDLEY_UNLIKELY(!ret.ptr)) {
        throw js_exception(JS_ThrowTypeError(c, "Argument %d is not of type %d", i, ret.id));
    }
    return ret;
}
template <> JSValue push_value_impl<impl_wrapped_class>(JSContext *c, const impl_wrapped_class &v) {
    auto r = JS_NewObjectClass(c, v.id);
    JS_SetOpaque(r, v.ptr);
    return r;
}

void *arg_list_helpers::this_getter::get(void *ctx, qjs::JSValue js_this, uint32_t id) {
    void *r = JS_GetOpaque2(static_cast<JSContext *>(ctx), js_this, id);
    if (HEDLEY_UNLIKELY(!r))
        throw js_exception(JS_ThrowTypeError(static_cast<JSContext *>(ctx), "this not of type %d", id));
    return r;
}

#define X(T)                                                                                                           \
    template <>                                                                                                        \
    HEDLEY_NON_NULL(1, 3)                                                                                              \
    HEDLEY_PURE T arg_list_helpers::getter<T>::get(                                                                    \
        void *HEDLEY_RESTRICT ctx, int argc, qjs::JSValue *HEDLEY_RESTRICT argv, int i, bool strict) {                 \
        return pull_value_impl<T>(static_cast<JSContext *>(ctx), argc, argv, i, strict);                               \
    }                                                                                                                  \
    template <>                                                                                                        \
    HEDLEY_NON_NULL(1, 3)                                                                                              \
    HEDLEY_PURE qjs::JSValue arg_list_helpers::setter<T>::set(void *ctx, const T &v) {                                 \
        return push_value_impl<T>(static_cast<JSContext *>(ctx), v);                                                   \
    }

#include "all_types.inl"
X(impl_wrapped_class);

#undef X

} // namespace jnjs::detail