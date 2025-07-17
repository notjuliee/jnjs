#pragma once

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#include <quickjs.h>

#include "fwd.h"
#include "hedley.h"
#include "type_traits.h"
#include "types.h"
#include "value_helpers.h"

namespace jnjs::detail {

struct js_exception : std::exception {
    explicit js_exception(JSValue v_) : v(v_) {}

    JSValue v;
};

struct arg_list_helpers {
    HEDLEY_PURE
    HEDLEY_NON_NULL(2)
    static bool is_null_or_undefined(int argc, JSValue *argv, int i) {
        return i >= argc || JS_IsNull(argv[i]) || JS_IsUndefined(argv[i]);
    }

    template <typename T, typename = void> struct getter {
        HEDLEY_NON_NULL(1, 3)
        static T get(JSContext *ctx, int argc, JSValue *argv, int i, bool strict) {
            using H = value_helpers<std::decay_t<T>>;
            if (HEDLEY_UNLIKELY(i >= argc)) {
                throw js_exception(JS_ThrowRangeError(ctx, "Argument out of range (%d >= %d)", i, argc));
            }
            if (!(strict ? H::is(ctx, argv[i]) : H::is_convertible(ctx, argv[i]))) {
                throw js_exception(JS_ThrowTypeError(ctx, "Argument %d is not of type %s", i, typeid(T).name()));
            }
            return H::as(ctx, argv[i]);
        }
    };
    template <typename T> struct getter<std::optional<T>> {
        HEDLEY_NON_NULL(1, 3)
        static std::optional<T> get(JSContext *ctx, int argc, JSValue *argv, int i, bool strict) {
            if (is_null_or_undefined(argc, argv, i))
                return std::nullopt;
            return getter<T>::get(ctx, argc, argv, i, strict);
        }
    };
    template <typename T> struct getter<must_be<T>> {
        HEDLEY_NON_NULL(1, 3)
        static T get(JSContext *ctx, int argc, JSValue *argv, int i, bool) {
            return getter<T>::get(ctx, argc, argv, i, true);
        }
    };
    template <typename T> struct getter<T, std::enable_if_t<has_build_v<remove_ref_cv_t<T>>>> {
        HEDLEY_NON_NULL(1, 3)
        static T &get(JSContext *ctx, int argc, JSValue *argv, int i, bool) {
            if (HEDLEY_UNLIKELY(i >= argc)) {
                throw js_exception(JS_ThrowRangeError(ctx, "Argument out of range (%d >= %d)", i, argc));
            }
            auto ptr = value_helpers<remove_ref_cv_t<T> *>::as(ctx, argv[i]);
            if (HEDLEY_UNLIKELY(ptr == nullptr)) {
                throw js_exception(JS_ThrowTypeError(ctx, "Argument %d is not of type %s", i, typeid(T).name()));
            }
            return *static_cast<remove_ref_cv_t<T> *>(ptr);
        }
    };
    template <typename T> struct getter<T *, std::enable_if_t<has_build_v<remove_ref_cv_t<T>>>> {
        HEDLEY_NON_NULL(1, 3)
        static T *get(JSContext *ctx, int argc, JSValue *argv, int i, bool) {
            auto ptr = value_helpers<remove_ref_cv_t<T> *>::as(ctx, argv[i]);
            return static_cast<remove_ref_cv_t<T> *>(ptr);
        }
    };
    struct this_getter {
        HEDLEY_PURE
        static void *get(JSValue js_this, uint32_t id) {
            void *r = JS_GetOpaque(js_this, id);
            return r;
        }
    };
    template <typename T> HEDLEY_NON_NULL(1, 3) static T get(JSContext *ctx, int argc, JSValue *argv, int i) {
        return getter<T>::get(ctx, argc, argv, i, false);
    }
    template <typename T> HEDLEY_NON_NULL(1) static JSValue set(JSContext *ctx, const T &v) {
        return value_helpers<T>::from(ctx, v);
    }
    template <typename T> static T *get_class(JSValue js_this) {
        return static_cast<T *>(this_getter::get(js_this, internal_class_meta<T>::data.id));
    }

    static void assert_called_new(JSContext *ctx, JSValue v) {
        if (HEDLEY_UNLIKELY(!get<must_be<bool>>(ctx, 1, &v, 0))) {
            throw js_exception(JS_ThrowTypeError(ctx, "Class constructor must be called with new"));
        }
    }
};

template <auto Func> struct binder {
    template <typename T> struct binder_inner;
    template <typename TRet, typename... TArgs> struct binder_inner<TRet (*)(TArgs...)> {
        using ret_type = getter_type_t<TRet>;
        using arg_types = std::tuple<TArgs...>;
        static constexpr size_t num_args = sizeof...(TArgs);

        template <std::size_t... Is>
        HEDLEY_NON_NULL(1, 3)
        HEDLEY_PURE static ret_type invoke(JSContext *ctx, int argc, JSValue *argv, std::index_sequence<Is...>) {
            return Func(std::forward<getter_type_t<TArgs> &&>(
                arg_list_helpers::get<getter_type_t<TArgs>>(ctx, argc, argv, Is))...);
        }

        template <typename TRetI = TRet>
        HEDLEY_NON_NULL(1, 3)
        static std::enable_if_t<!std::is_same_v<TRetI, void>, JSValue> call_impl(
            JSContext *ctx, int argc, JSValue *argv) {
            return arg_list_helpers::set<ret_type>(ctx, invoke(ctx, argc, argv, std::make_index_sequence<num_args>{}));
        }
        template <typename TRetI = TRet>
        static std::enable_if_t<std::is_same_v<TRetI, void>, JSValue> call_impl(
            JSContext *ctx, int argc, JSValue *argv) {
            invoke(ctx, argc, argv, std::make_index_sequence<num_args>{});
            return arg_list_helpers::set<undefined>(ctx, undefined{});
        }
    };

    using inner = binder_inner<decltype(Func)>;
    static constexpr size_t num_args = inner::num_args;
    HEDLEY_NON_NULL(1, 4)
    static JSValue call(JSContext *ctx, JSValue, int argc, JSValue *argv) {
        try {
            return inner::call_impl(ctx, argc, argv);
        } catch (js_exception &e) {
            return e.v;
        }
    }

    HEDLEY_NON_NULL(1, 5)
    static JSValue call_ctor_t(JSContext *ctx, JSValue func_obj, JSValue js_this, int argc, JSValue *argv, int flags) {
        (void)func_obj; // idk what use we'd have for this
        (void)flags;    // idk what this even does
        try {
            arg_list_helpers::assert_called_new(ctx, js_this);
            return inner::call_impl(ctx, argc, argv);
        } catch (js_exception &e) {
            return e.v;
        }
    }
};

template <typename Klass, auto Func> struct class_binder {
    template <typename T> struct binder_inner;
    template <typename TRet, typename... TArgs> struct binder_inner<TRet (Klass::*)(TArgs...)> {
        using ret_type = getter_type_t<TRet>;
        using arg_types = std::tuple<TArgs...>;
        static constexpr size_t num_args = sizeof...(TArgs);

        template <std::size_t... Is>
        HEDLEY_NON_NULL(1, 4)
        static ret_type invoke(JSContext *ctx, JSValue js_this, int argc, JSValue *argv, std::index_sequence<Is...>) {
            Klass *kThis = arg_list_helpers::get_class<Klass>(js_this);
            return (kThis->*Func)(std::forward<getter_type_t<TArgs> &&>(
                arg_list_helpers::get<getter_type_t<TArgs>>(ctx, argc, argv, Is))...);
        }

        template <typename TRetI = TRet>
        HEDLEY_NON_NULL(1, 4)
        static std::enable_if_t<!std::is_same_v<TRetI, void>, JSValue> call_impl(
            JSContext *ctx, JSValue js_this, int argc, JSValue *argv) {
            return arg_list_helpers::set<ret_type>(
                ctx, invoke(ctx, js_this, argc, argv, std::make_index_sequence<num_args>{}));
        }
        template <typename TRetI = TRet>
        HEDLEY_NON_NULL(1, 4)
        static std::enable_if_t<std::is_same_v<TRetI, void>, JSValue> call_impl(
            JSContext *ctx, JSValue js_this, int argc, JSValue *argv) {
            invoke(ctx, js_this, argc, argv, std::make_index_sequence<num_args>{});
            return arg_list_helpers::set<undefined>(ctx, undefined{});
        }
    };

    using inner = binder_inner<decltype(Func)>;
    using ret_type = typename inner::ret_type;
    static constexpr size_t num_args = inner::num_args;
    HEDLEY_NON_NULL(1, 4)
    static JSValue call(JSContext *ctx, JSValue js_this, int argc, JSValue *argv) {
        try {
            return inner::call_impl(ctx, js_this, argc, argv);
        } catch (js_exception &e) {
            return e.v;
        }
    }

    HEDLEY_NON_NULL(1)
    static JSValue call_get(JSContext *ctx, JSValue js_this) {
        static_assert(num_args == 0, "getter must have 0 arguments");
        static_assert(!std::is_same_v<ret_type, void>, "getter can't return void");
        try {
            return inner::call_impl(ctx, js_this, 0, &js_this);
        } catch (js_exception &e) {
            return e.v;
        }
    }

    HEDLEY_NON_NULL(1)
    static JSValue call_set(JSContext *ctx, JSValue js_this, JSValue arg) {
        static_assert(num_args == 1, "setter must have 1 argument");
        try {
            return inner::call_impl(ctx, js_this, 1, &arg);
        } catch (js_exception &e) {
            return e.v;
        }
    }
};

template <typename T, typename... Args> using ctor_fn_t = T *(*)(Args...);
template <typename T> using dtor_fn_t = void (*)(T *);

} // namespace jnjs::detail
