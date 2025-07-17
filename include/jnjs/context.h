#pragma once

#ifndef JNJS_IMPL_USING_MODULE
#include <memory>
#include <quickjs.h>
#include <string_view>

#include "binding.h"
#include "value.h"

#include "detail/function_helpers.h"
#include "detail/util.h"
#endif

namespace jnjs {

/**
 * @brief javascript context
 */
class context : detail::impl_ptr<JSContext> {
    using base = detail::impl_ptr<JSContext>;

  public:
    ~context() = default;

    value eval(std::string_view code) {
        auto v = JS_Eval(get(), code.data(), code.size(), "<eval>", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
        return value(v, get());
    }

    template <typename T> void set_global(const char *name, const T &v) {
        _set_global(name, detail::value_helpers<T>::from(get(), v));
    }

    template <auto Func> void set_global_fn(const char *name) {
        using helper = detail::binder<Func>;
        set_global(name, function(get(), name, helper::call, helper::num_args));
    }

    template <typename K, typename = std::enable_if_t<detail::has_build_v<K>, void>> void install_class() {
        constexpr static wrapped_class_builder<K> binder = K::build_js_class();
        return _decl_class_impl(binder._d, detail::internal_class_meta<K>::data);
    }

  private:
    explicit context(JSRuntime &rt) : base(JS_NewContext(&rt), JS_FreeContext) {}
    void _set_global(const char *name, JSValue v) {
        auto ctx = get();
        auto g = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, g, name, v);
        JS_FreeValue(ctx, g);
    }
    value _make_cfunc_value(const char *name, JSCFunction *fn, int len) const;
    void _decl_class_impl(const detail::class_builder_data &, detail::internal_class_meta_data &o);
    friend runtime;
};

} // namespace jnjs