#pragma once

#include "detail/function_helpers.h"
#include "detail/fwd.h"
#include "detail/util.h"

#include <array>

namespace jnjs {

namespace detail {

struct class_builder_data {
    constexpr static uint8_t max_function_count = 64;
    uint8_t cur_fn = 0;
    JSCFunctionListEntry fns[max_function_count] = {};
    JSClassDef def = {};
    JSCFunction *ctor = nullptr;
    int ctor_len = 0;
};

} // namespace detail

/**
 * @brief builder to generate bindings from a C++ class to
 * @tparam Klass the class to bind
 */
template <typename Klass> struct wrapped_class_builder {
    /**
     * @brief create a new class builder
     * @param name bound class name
     */
    constexpr explicit wrapped_class_builder(const char *name, detail::key<Klass> = {}) { _d.def.class_name = name; }

    /**
     * @brief bind an instance method
     * @tparam Func function to bind
     * @param name function name
     */
    template <auto Func> constexpr void bind_function(const char *name) {
        using binder = detail::class_binder<Klass, Func>;
        auto &fn = _next_entry(name);
        fn.u.func.length = binder::num_args;
        fn.u.func.cproto = JS_CFUNC_generic;
        fn.u.func.cfunc.generic = binder::call;
    }

    /**
     * @brief bind a getter
     * @note function must return a value, and have no parameters
     * @warning do not use with bind_setter, if you need both use bind_getset
     * @tparam Func getter function
     * @param name property name
     */
    template <auto Func> constexpr void bind_getter(const char *name) {
        using binder = detail::class_binder<Klass, Func>;
        auto &fn = _next_entry(name);
        fn.def_type = JS_DEF_CGETSET;
        fn.u.getset = {};
        fn.u.getset.get.getter = binder::call_get;
        fn.u.getset.set.setter = nullptr;
    }

    /**
     * @brief bind a setter
     * @note function must not return a value, and take one argument
     * @warning do not use with bind_getter, if you need both use bind_getset
     * @tparam Func setter function
     * @param name property name
     */
    template <auto Func> constexpr void bind_setter(const char *name) {
        using binder = detail::class_binder<Klass, Func>;
        auto &fn = _next_entry(name);
        fn.def_type = JS_DEF_CGETSET;
        fn.u.getset = {};
        fn.u.getset.get.getter = nullptr;
        fn.u.getset.set.setter = binder::call_set;
    }

    /**
     * @brief bind both a getter and a setter
     * @see bind_getter
     * @see bind_setter
     * @tparam FuncG getter function
     * @tparam FuncS setter function
     * @param name property name
     */
    template <auto FuncG, auto FuncS> constexpr void bind_getset(const char *name) {
        using binder_g = detail::class_binder<Klass, FuncG>;
        using binder_s = detail::class_binder<Klass, FuncS>;
        auto &fn = _next_entry(name);
        fn.def_type = JS_DEF_CGETSET;
        fn.u.getset = {};
        fn.u.getset.get.getter = binder_g::call_get;
        fn.u.getset.set.setter = binder_s::call_set;
    }

    /**
     * @brief bind a constructor
     * @note only one constructor can be bound
     * @tparam Args construct argument types
     */
    template <typename... Args> constexpr void bind_ctor() {
        _bind_dtor();
        using helper = ctor_helper<Args...>;
        using binder = detail::binder<helper::call>;
        _d.ctor = binder::call;
    }

  private:
    constexpr void _bind_dtor() {
        if (_d.def.finalizer)
            return;
        _d.def.finalizer = dtor_helper::call;
    }

    constexpr JSCFunctionListEntry &_next_entry(const char *name) {
        auto &fn = _d.fns[_d.cur_fn++];
        fn.name = name;
        fn.prop_flags = JS_PROP_ENUMERABLE;
        fn.def_type = JS_DEF_CFUNC;
        return fn;
    }

    template <typename... Args> struct ctor_helper {
        static Klass *call(Args &&...args) { return new Klass(std::forward<Args &&>(args)...); }
    };
    struct dtor_helper {
        static void call(JSRuntime *, JSValue v) {
            auto *t = detail::arg_list_helpers::get_class<Klass>(v);
            delete t;
        }
    };

  private:
    detail::class_builder_data _d = {};
    friend runtime;
    friend context;
    friend module;
};

} // namespace jnjs
