#include <quickjs.h>

#include <jnjs/context.h>

#include "value_helpers.h"

namespace jnjs {

namespace detail {
namespace {
void install_rt_class(JSRuntime *rt, const class_builder_data &d, detail::internal_class_meta_data &o) {
    if (o.id != 0 && o.ctx == rt)
        return;
    JS_NewClassID(rt, &o.id);
    JS_NewClass(rt, o.id, reinterpret_cast<const JSClassDef *>(&d.def));
    o.ctx = rt;
}
} // namespace
} // namespace detail

context::context(detail::qjs::JSRuntime &rt) : base(JS_NewContext(&rt), JS_FreeContext) {}

void context::set_global(const char *name, const value &v) {
    auto ctx = get();
    auto g = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, g, name, JS_DupValue(ctx, v._v));
    JS_FreeValue(ctx, g);
}

value context::_make_cfunc_value(const char *name, void *fn, int len) const {
    auto ctx = get();
    const auto v = JS_NewCFunction(ctx, reinterpret_cast<JSCFunction *>(fn), name, len);
    return value(v, ctx);
}

value context::eval(std::string_view code) {
    auto v = JS_Eval(get(), code.data(), code.size(), "<eval>", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
    return value(v, get());
}

#define X(T)                                                                                                           \
    template <> value context::make_value<T>(const T &v) const {                                                       \
        return detail::impl_value_helpers::make_value_t<T>(get(), v);                                                  \
    }
#include "all_types.inl"
#undef X

void context::_decl_class_impl(const detail::class_builder_data &d, detail::internal_class_meta_data &oid) {
    auto *ctx = get();
    detail::install_rt_class(JS_GetRuntime(ctx), d, oid);

    auto proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, reinterpret_cast<const JSCFunctionListEntry *>(d.fns.data()), d.cur_fn);

    if (d.ctor) {
        JSValue ctor = JS_NewCFunction2(
            ctx, reinterpret_cast<JSCFunction *>(d.ctor), d.def.class_name, d.ctor_len, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        set_global(d.def.class_name, value(ctor, ctx));
    }

    JS_SetClassProto(ctx, oid.id, proto);
}

value context::_make_class_value(uint32_t id, void *ptr) const {
    auto *ctx = get();
    auto v = JS_NewObjectClass(ctx, id);
    JS_SetOpaque(v, ptr);
    return value(v, ctx);
}

} // namespace jnjs