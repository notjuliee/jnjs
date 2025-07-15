#pragma once

#include "detail/function_helpers.h"
#include "detail/fwd.h"
#include "detail/util.h"

#include <array>

namespace jnjs {

namespace detail {
constexpr uint8_t max_function_count = 64;

struct class_builder_data {
    uint8_t cur_fn = 0;
    std::array<qjs::JSCFunctionListEntry, max_function_count> fns = {};
    qjs::JSClassDef def = {};
    qjs::fn_generic ctor = nullptr;
    int ctor_len = 0;
};
} // namespace detail

template <typename Klass> struct wrapped_class_builder {
    constexpr explicit wrapped_class_builder(const char *name, detail::key<Klass> = {}) { _d.def.class_name = name; }

    template <auto Func> constexpr void bind_function(const char *name) {
        using binder = detail::class_binder<Klass, Func>;
        auto &fn = _next_entry(name);
        fn.func.length = binder::num_args;
        fn.func.cproto = detail::qjs::fn_type_which::generic;
        fn.func.fn.generic = binder::call;
    }

    template <auto Func> constexpr void bind_getter(const char *name) {
        using binder = detail::class_binder<Klass, Func>;
        auto &fn = _next_entry(name);
        fn.def_type = detail::qjs::fn_def_type::getset;
        fn.getset = {};
        fn.getset.getter = binder::call_get;
        fn.getset.setter = nullptr;
    }

    template <auto Func> constexpr void bind_setter(const char *name) {
        using binder = detail::class_binder<Klass, Func>;
        auto &fn = _next_entry(name);
        fn.def_type = detail::qjs::fn_def_type::getset;
        fn.getset = {};
        fn.getset.getter = nullptr;
        fn.getset.setter = binder::call;
    }

    template <auto FuncG, auto FuncS> constexpr void bind_getset(const char *name) {
        using binder_g = detail::class_binder<Klass, FuncG>;
        using binder_s = detail::class_binder<Klass, FuncS>;
        auto &fn = _next_entry(name);
        fn.def_type = detail::qjs::fn_def_type::getset;
        fn.getset = {};
        fn.getset.getter = binder_g::call_get;
        fn.getset.setter = binder_s::call_set;
    }

    template <typename... Args> constexpr void bind_ctor() {
        _bind_dtor();
        using helper = ctor_helper<Args...>;
        using binder = detail::binder<helper::call>;
        _d.ctor = binder::call;
    }

  private:
    constexpr void _bind_dtor() {
        if (_d.def.dtor)
            return;
        _d.def.dtor = dtor_helper::call;
    }

    constexpr detail::qjs::JSCFunctionListEntry &_next_entry(const char *name) {
        auto &fn = _d.fns.at(_d.cur_fn++);
        fn = {};
        fn.name = name;
        fn.prop_flags = detail::qjs::fn_prop_flags::enumerable;
        fn.def_type = detail::qjs::fn_def_type::function;
        return fn;
    }

    template <typename... Args> struct ctor_helper {
        static detail::impl_wrapped_class call(Args &&...args) {
            return detail::impl_wrapped_class::from(new Klass(std::forward<Args &&>(args)...));
        }
    };
    struct dtor_helper {
        static void call(detail::qjs::JSContext *c, detail::qjs::JSValue v) {
            auto *t = detail::arg_list_helpers::get_class<Klass>(c, v);
            delete t;
        }
    };

  private:
    detail::class_builder_data _d = {};
    friend runtime;
    friend context;
};

} // namespace jnjs
