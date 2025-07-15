#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

#include "hedley.h"
#include "quickjs_stub.h"
#include "types.h"

#include <functional>
#include <type_traits>

namespace jnjs {
template <typename T> struct wrapped_class_builder;
}

namespace jnjs::detail {

template <typename T> using remove_ref_cv_t = std::remove_reference_t<std::remove_cv_t<T>>;

template <typename T>
constexpr bool has_build_v<T, std::void_t<decltype(&T::build)>> =
    std::is_same_v<decltype(&T::build), wrapped_class_builder<T> (*)()>;

template <typename T> struct getter_type {
    using type = remove_ref_cv_t<T>;
};

template <typename T> struct getter_type<T &> {
    using Tb = remove_ref_cv_t<T>;
    using type = std::conditional_t<has_build_v<Tb>, T &, Tb>;
};
template <typename T> struct getter_type<const T &> {
    using Tb = remove_ref_cv_t<T>;
    using type = std::conditional_t<has_build_v<Tb>, T &, Tb>;
};
template <typename T> struct getter_type<T *> {
    static_assert(has_build_v<T>, "Cannot get pointer to non-class type");
    using type = T *;
};
template <typename T> struct getter_type<const T *> {
    static_assert(has_build_v<T>, "Cannot get pointer to non-class type");
    using type = T *;
};
template <typename T> using getter_type_t = typename getter_type<T>::type;

struct js_exception : std::exception {
    explicit js_exception(qjs::JSValue v_) : v(v_) {}

    qjs::JSValue v;
};

struct arg_list_helpers {
    static bool is_null_or_undefined(int argc, qjs::JSValue *argv, int i);

    template <typename T, typename = void> struct getter {
        HEDLEY_PURE
        HEDLEY_NON_NULL(1, 3)
        static T get(void *HEDLEY_RESTRICT ctx, int argc, qjs::JSValue *HEDLEY_RESTRICT argv, int i, bool strict);
    };
    template <typename T> struct getter<std::optional<T>> {
        HEDLEY_PURE
        HEDLEY_ALWAYS_INLINE
        HEDLEY_NON_NULL(1, 3)
        static std::optional<T> get(
            void *HEDLEY_RESTRICT ctx, int argc, qjs::JSValue *HEDLEY_RESTRICT argv, int i, bool strict) {
            if (is_null_or_undefined(argc, argv, i))
                return std::nullopt;
            return getter<T>::get(ctx, argc, argv, i, strict);
        }
    };
    template <typename T> struct getter<must_be<T>> {
        HEDLEY_PURE
        HEDLEY_ALWAYS_INLINE
        HEDLEY_NON_NULL(1, 3)
        static T get(void *HEDLEY_RESTRICT ctx, int argc, qjs::JSValue *HEDLEY_RESTRICT argv, int i, bool) {
            return getter<T>::get(ctx, argc, argv, i, true);
        }
    };
    template <typename T> struct getter<T, std::enable_if_t<has_build_v<remove_ref_cv_t<T>>>> {
        HEDLEY_PURE
        HEDLEY_NON_NULL(1, 3)
        static T &get(void *ctx, int argc, qjs::JSValue *argv, int i, bool) {
            auto [ptr, id] = getter<impl_wrapped_class>::get(ctx, argc, argv, i, true);
            if (HEDLEY_UNLIKELY(id != internal_class_meta<std::decay_t<T>>::data.id)) {
                asserters::assertion_failure(ctx, "Argument has incorrect type");
            }
            return *static_cast<remove_ref_cv_t<T> *>(ptr);
        }
    };
    template <typename T> struct getter<T *, std::enable_if_t<has_build_v<remove_ref_cv_t<T>>>> {
        HEDLEY_PURE
        HEDLEY_NON_NULL(1, 3)
        static T *get(void *ctx, int argc, qjs::JSValue *argv, int i, bool) {
            auto [ptr, id] = getter<impl_wrapped_class>::get(ctx, argc, argv, i, false);
            if (id != 0 && HEDLEY_UNLIKELY(id != internal_class_meta<std::decay_t<T>>::data.id)) {
                asserters::assertion_failure(ctx, "Argument has incorrect type");
            }
            return static_cast<remove_ref_cv_t<T> *>(ptr);
        }
    };
    template <typename T> struct setter {
        HEDLEY_NON_NULL(1)
        static qjs::JSValue set(void *ctx, const T &v);
    };
    struct this_getter {
        HEDLEY_PURE
        HEDLEY_NON_NULL(1)
        static void *get(void *ctx, qjs::JSValue js_this, uint32_t id);
    };
    struct asserters {
        HEDLEY_NON_NULL(1, 2)
        HEDLEY_NO_RETURN
        static void assertion_failure(void *HEDLEY_RESTRICT ctx, const char *HEDLEY_RESTRICT msg);
    };
    HEDLEY_PURE
    HEDLEY_NON_NULL(1, 3)
    template <typename T> static T get(qjs::JSContext *ctx, int argc, qjs::JSValue *argv, int i) {
        return getter<T>::get(ctx, argc, argv, i, false);
    }
    HEDLEY_NON_NULL(1)
    template <typename T> static qjs::JSValue set(qjs::JSContext *ctx, const T &v) { return setter<T>::set(ctx, v); }
    HEDLEY_PURE
    template <typename T> static T *get_class(qjs::JSContext *ctx, qjs::JSValue js_this) {
        return static_cast<T *>(this_getter::get(ctx, js_this, internal_class_meta<T>::data.id));
    }

    static void assert_called_new(qjs::JSContext *ctx, qjs::JSValue v) {
        if (HEDLEY_UNLIKELY(!get<must_be<bool>>(ctx, 1, &v, 0))) {
            asserters::assertion_failure(ctx, "Class constructor must be called with new");
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
        HEDLEY_PURE static ret_type
            invoke(qjs::JSContext *ctx, int argc, qjs::JSValue *argv, std::index_sequence<Is...>) {
            return Func(std::forward<getter_type_t<TArgs> &&>(
                arg_list_helpers::get<getter_type_t<TArgs>>(ctx, argc, argv, Is))...);
        }

        template <typename TRetI = TRet>
        HEDLEY_NON_NULL(1, 3)
        static std::enable_if_t<!std::is_same_v<TRetI, void>, qjs::JSValue> call_impl(
            qjs::JSContext *ctx, int argc, qjs::JSValue *argv) {
            return arg_list_helpers::set<ret_type>(ctx, invoke(ctx, argc, argv, std::make_index_sequence<num_args>{}));
        }
        template <typename TRetI = TRet>
        static std::enable_if_t<std::is_same_v<TRetI, void>, qjs::JSValue> call_impl(
            qjs::JSContext *ctx, int argc, qjs::JSValue *argv) {
            invoke(ctx, argc, argv, std::make_index_sequence<num_args>{});
            return arg_list_helpers::set<undefined>(ctx, undefined{});
        }
    };

    using inner = binder_inner<decltype(Func)>;
    static constexpr size_t num_args = inner::num_args;
    HEDLEY_NON_NULL(1, 3)
    static qjs::JSValue call(qjs::JSContext *ctx, qjs::JSValue, int argc, qjs::JSValue *argv) {
        try {
            return inner::call_impl(ctx, argc, argv);
        } catch (js_exception &e) {
            return e.v;
        }
    }

    HEDLEY_NON_NULL(1, 5)
    static qjs::JSValue call_ctor_t(
        qjs::JSContext *ctx, qjs::JSValue func_obj, qjs::JSValue js_this, int argc, qjs::JSValue *argv, int flags) {
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
        HEDLEY_NON_NULL(1, 3)
        HEDLEY_PURE static ret_type invoke(
            qjs::JSContext *ctx, qjs::JSValue js_this, int argc, qjs::JSValue *argv, std::index_sequence<Is...>) {
            Klass *kThis = arg_list_helpers::get_class<Klass>(ctx, js_this);
            return (kThis->*Func)(std::forward<getter_type_t<TArgs> &&>(
                arg_list_helpers::get<getter_type_t<TArgs>>(ctx, argc, argv, Is))...);
        }

        template <typename TRetI = TRet>
        HEDLEY_NON_NULL(1, 4)
        static std::enable_if_t<!std::is_same_v<TRetI, void>, qjs::JSValue> call_impl(
            qjs::JSContext *ctx, qjs::JSValue js_this, int argc, qjs::JSValue *argv) {
            return arg_list_helpers::set<ret_type>(
                ctx, invoke(ctx, js_this, argc, argv, std::make_index_sequence<num_args>{}));
        }
        template <typename TRetI = TRet>
        HEDLEY_NON_NULL(1, 4)
        HEDLEY_PURE static std::enable_if_t<std::is_same_v<TRetI, void>, qjs::JSValue> call_impl(
            qjs::JSContext *ctx, qjs::JSValue js_this, int argc, qjs::JSValue *argv) {
            invoke(ctx, js_this, argc, argv, std::make_index_sequence<num_args>{});
            return arg_list_helpers::set<undefined>(ctx, undefined{});
        }
    };

    using inner = binder_inner<decltype(Func)>;
    using ret_type = typename inner::ret_type;
    static constexpr size_t num_args = inner::num_args;
    HEDLEY_NON_NULL(1, 4)
    static qjs::JSValue call(qjs::JSContext *ctx, qjs::JSValue js_this, int argc, qjs::JSValue *argv) {
        try {
            return inner::call_impl(ctx, js_this, argc, argv);
        } catch (js_exception &e) {
            return e.v;
        }
    }

    HEDLEY_NON_NULL(1)
    static qjs::JSValue call_get(qjs::JSContext *ctx, qjs::JSValue js_this) {
        static_assert(num_args == 0, "getter must have 0 arguments");
        static_assert(!std::is_same_v<ret_type, void>, "getter can't return void");
        try {
            return inner::call_impl(ctx, js_this, 0, nullptr);
        } catch (js_exception &e) {
            return e.v;
        }
    }

    HEDLEY_NON_NULL(1)
    static qjs::JSValue call_set(qjs::JSContext *ctx, qjs::JSValue js_this, qjs::JSValue arg) {
        static_assert(num_args == 1, "setter must have 1 argument");
        try {
            return inner::call_impl(ctx, js_this, 1, &arg);
        } catch (js_exception &e) {
            return e.v;
        }
    }
};

template <typename T, typename... Args> using ctor_fn_t = impl_wrapped_class (*)(Args...);
template <typename T> using dtor_fn_t = void (*)(T *);

} // namespace jnjs::detail
