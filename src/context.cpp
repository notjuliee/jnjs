#include <quickjs.h>

#include <jnjs/context.h>

namespace jnjs {

namespace detail {
namespace {
void install_rt_class(JSRuntime *rt, const class_builder_data &d, internal_class_meta_data &o) {
    if (o.id != 0)
        return;
    JS_NewClassID(rt, &o.id);
    JS_NewClass(rt, o.id, &d.def);
}
} // namespace
} // namespace detail

void context::_decl_class_impl(const detail::class_builder_data &d, detail::internal_class_meta_data &oid) {
    auto *ctx = get();
    detail::install_rt_class(JS_GetRuntime(ctx), d, oid);

    auto proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, d.fns, d.cur_fn);

    if (d.ctor) {
        JSValue ctor = JS_NewCFunction2(ctx, d.ctor, d.def.class_name, d.ctor_len, JS_CFUNC_constructor, 0);
        JS_SetConstructor(ctx, ctor, proto);
        set_global(d.def.class_name, value(ctor, ctx));
    }

    JS_SetClassProto(ctx, oid.id, proto);
}

} // namespace jnjs