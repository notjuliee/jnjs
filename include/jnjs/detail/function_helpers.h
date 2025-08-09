#pragma once
/**
 * @file function_helpers.h
 * @brief Helpers for binding C++ functions to JavaScript.
 * @internal
 */

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <quickjs.h>

#include "fwd.h"
#include "hedley.h"
#include "type_traits.h"
#include "types.h"
#include "value_helpers.h"

namespace jnjs {
/**
 * @brief A vector type that binds to the remaining arguments in a function call.
 * @tparam T Type of elements to bind to.
 */
template <typename T> struct remaining_args final : std::vector<T> {
    using std::vector<T>::vector;
};
} // namespace jnjs

/**
 * @internal
 */
namespace jnjs::detail {

/**
 * @internal
 * @brief Exception class for JavaScript errors.
 *
 * This class is used to throw exceptions when a JavaScript operation fails,
 * encapsulating a JSValue that represents the error.
 */
struct js_exception final : std::exception {
    explicit js_exception(const JSValue v_) : v(v_) {}

    JSValue v;
};

/**
 * @internal
 * @brief Helpers for argument lists in function calls.
 *
 * This namespace contains functions and classes to handle argument lists
 * passed to JavaScript functions, including type checking and conversion.
 */
namespace arg_list_helpers {
/**
 * @internal
 * @brief Check if the argument at index `i` is null or undefined.
 *
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @param i Index to check
 * @return true if the argument is null or undefined, false otherwise
 */
HEDLEY_PURE
HEDLEY_NON_NULL(2)
static bool is_null_or_undefined(int argc, JSValue *argv, int i) {
    return i >= argc || JS_IsNull(argv[i]) || JS_IsUndefined(argv[i]);
}

/**
 * @internal
 * @brief Helper to get a value of type T from a JavaScript argument list.
 * @tparam T The type of the value to get.
 */
template <typename T, typename = void> struct getter {
    /**
     * @internal
     * @brief Get a value of type T from a JavaScript argument list.
     * @param ctx The current JavaScript context.
     * @param argc The number of arguments passed to the function.
     * @param argv The array of arguments passed to the function.
     * @param i The index of the argument to get.
     * @return The value at index `i` converted to type T.
     * @throws js_exception if the argument is out of range or not convertible to type T.
     */
    HEDLEY_NON_NULL(1, 3)
    static T get(JSContext *ctx, int argc, JSValue *argv, int i) {
        using H = value_helpers<std::decay_t<T>>;
        if (HEDLEY_UNLIKELY(i >= argc)) {
            throw js_exception(JS_ThrowRangeError(ctx, "Argument out of range (%d >= %d)", i, argc));
        }
        if (!H::is_convertible(ctx, argv[i])) {
            throw js_exception(JS_ThrowTypeError(ctx, "Argument %d is not of type %s", i, typeid(T).name()));
        }
        return H::as(ctx, argv[i]);
    }
};

/**
 * @internal
 * @brief Get an optional value of type T from a JavaScript argument list.
 * This specialization allows for arguments that may be null, undefined, or not provided.
 * @tparam T The type of the value to get.
 */
template <typename T> struct getter<std::optional<T>> {
    HEDLEY_NON_NULL(1, 3)
    static std::optional<T> get(JSContext *ctx, int argc, JSValue *argv, int i) {
        if (is_null_or_undefined(argc, argv, i))
            return std::nullopt;
        return getter<T>::get(ctx, argc, argv, i);
    }
};

/**
 * @internal
 * @brief Get a reference to a C++ class instance from a JavaScript argument list.
 * @tparam T A class type that has been registered with the context.
 */
template <typename T> struct getter<T, std::enable_if_t<has_build_v<remove_ref_cv_t<T>>>> {
    HEDLEY_NON_NULL(1, 3)
    static T &get(JSContext *ctx, int argc, JSValue *argv, int i) {
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
/**
 * @internal
 * @brief Get a pointer to a C++ class instance from a JavaScript argument list, allowing for null pointers.
 * @tparam T A class type that has been registered with the context.
 */
template <typename T> struct getter<T *, std::enable_if_t<has_build_v<remove_ref_cv_t<T>>>> {
    HEDLEY_NON_NULL(1, 3)
    static T *get(JSContext *ctx, int argc, JSValue *argv, int i) {
        if (i >= argc) {
            return nullptr; // Allow null pointers
        }
        auto ptr = value_helpers<remove_ref_cv_t<T> *>::as(ctx, argv[i]);
        return static_cast<remove_ref_cv_t<T> *>(ptr);
    }
};

/**
 * @internal
 * @brief Get all remaining arguments from a JavaScript argument list.
 * @tparam T The type of values to get.
 */
template <typename T> struct getter<remaining_args<T>> {
    HEDLEY_NON_NULL(1, 3)
    static remaining_args<T> get(JSContext *ctx, int argc, JSValue *argv, int i) {
        if (HEDLEY_UNLIKELY(i >= argc)) {
            return {};
        }
        remaining_args<T> ret(argc - i);
        for (int j = i; j < argc; ++j) {
            ret.at(j - i) = std::move(getter<T>::get(ctx, argc, argv, j));
        }
        return ret;
    }
};

/**
 * @internal
 * @brief Specialization to avoid copying JSValue objects from the argument list.
 */
template <> struct getter<JSValue> {
    static JSValue get(JSContext *ctx, int argc, JSValue *argv, int i) {
        if (HEDLEY_UNLIKELY(i >= argc)) {
            throw js_exception(JS_ThrowRangeError(ctx, "Argument out of range (%d >= %d)", i, argc));
        }
        return argv[i];
    }
};

/**
 * @internal
 * @brief Helper to create a new JSValue from a C++ value.
 * @tparam T The type of the value to set.
 */
template <typename T> struct setter {
    /**
     * @brief Set a value of type T in a JavaScript context.
     * @param ctx The current JavaScript context.
     * @param v The value to set.
     * @return A JSValue representing the value.
     */
    HEDLEY_NON_NULL(1)
    static JSValue set(JSContext *ctx, const T &v) { return value_helpers<T>::from(ctx, v); }
};

/**
 * @internal
 * @brief Specialization to avoid copying JSValue objects when returning.
 */
template <> struct setter<JSValue> {
    HEDLEY_NON_NULL(1)
    static JSValue set(JSContext *, const JSValue &v) { return v; }
};

/**
 * @internal
 * @brief Helper to get a C++ class instance from a JS this object.
 */
struct this_getter {
    /**
     * @internal
     * @brief Get a C++ class instance from a JS this object.
     * @param js_this JS object representing this.
     * @param id The internal class ID to look for.
     * @return A pointer to the C++ class instance, or nullptr if not found.
     */
    HEDLEY_PURE
    static void *get(JSValue js_this, uint32_t id) {
        void *r = JS_GetOpaque(js_this, id);
        return r;
    }
};

/**
 * @internal
 * @brief Get a value of type T from a JavaScript argument list at a specific index.
 * @tparam T Type of the value to get.
 * @param ctx Current JavaScript context.
 * @param argc Number of arguments passed to the function.
 * @param argv Array of arguments passed to the function.
 * @param i Index of the argument to get.
 * @return The value at index `i` converted to type T.
 * @throws js_exception if the argument is out of range or not convertible to type T.
 */
template <typename T>
#ifndef DOXYGEN
HEDLEY_NON_NULL(1, 3)
#endif
static T get(JSContext *ctx, int argc, JSValue *argv, int i) {
    return getter<T>::get(ctx, argc, argv, i);
}
/**
 * @internal
 * @brief Set a value of type T in a JavaScript context.
 * @tparam T Type of the value to set.
 * @param ctx Current JavaScript context.
 * @param v Value to set.
 * @return A JSValue representing the value.
 */
template <typename T>
#ifndef DOXYGEN
HEDLEY_NON_NULL(1)
#endif
static JSValue set(JSContext *ctx, const T &v) {
    return setter<T>::set(ctx, v);
}
/**
 * @internal
 * @brief Get a C++ class instance from a JS this object.
 * @tparam T Class of expected value of this
 * @param js_this Value of this in a JS function call
 * @return Pointer to the C++ class instance associated with this JS object, or nullptr if not found.
 */
template <typename T> static T *get_class(JSValue js_this) {
    return static_cast<T *>(this_getter::get(js_this, internal_class_meta<T>::data.id));
}

/**
 * @internal
 * @brief Assert that a function was called with `new`.
 * @param ctx Current JavaScript context.
 * @param v JSValue to check if was called with `new`.
 * @throws js_exception if the function was not called with `new`.
 */
static void assert_called_new(JSContext *ctx, JSValue v) {
    if (HEDLEY_UNLIKELY(!get<must_be<bool>>(ctx, 1, &v, 0))) {
        throw js_exception(JS_ThrowTypeError(ctx, "Class constructor must be called with new"));
    }
}
} // namespace arg_list_helpers

/**
 * @brief Binder for a function.
 * @tparam Func Address of the function to bind.
 */
template <auto Func> struct binder {
    /**
     * @internal
     * @brief Forward declaration of the inner binder type, to allow destructuring of the function signature.
     * @tparam T Type of the function
     */
    template <typename T> struct binder_inner;
    /**
     * @internal
     * @brief Binder for a function pointer type.
     * @tparam TRet Return type of the function.
     * @tparam TArgs Argument types of the function.
     */
    template <typename TRet, typename... TArgs> struct binder_inner<TRet (*)(TArgs...)> {
        using ret_type = getter_type_t<TRet>;
        using arg_types = std::tuple<TArgs...>;
        static constexpr size_t num_args = sizeof...(TArgs);

        /**
         * @internal
         * @brief Invoke `Func` with the provided arguments.
         * @tparam Is Indices of the arguments to pass to the function.
         * @param ctx Current JavaScript context.
         * @param argc Number of arguments passed to the function.
         * @param argv Array of arguments passed to the function.
         * @return Return value of `Func`.
         */
        template <std::size_t... Is>
        HEDLEY_NON_NULL(1, 3)
        HEDLEY_PURE static ret_type invoke(JSContext *ctx, int argc, JSValue *argv, std::index_sequence<Is...>) {
            return Func(std::forward<getter_type_t<TArgs> &&>(
                arg_list_helpers::get<getter_type_t<TArgs>>(ctx, argc, argv, Is))...);
        }

        /**
         * @internal
         * @brief Call the function with the provided arguments and return null.
         * @param ctx Current JavaScript context.
         * @param argc Argument count.
         * @param argv Argument list.
         * @return Undefined JSValue.
         */
        template <typename TRetI = TRet>
        HEDLEY_NON_NULL(1, 3)
        static std::enable_if_t<!std::is_same_v<TRetI, void>, JSValue> call_impl(
            JSContext *ctx, int argc, JSValue *argv) {
            return arg_list_helpers::set<ret_type>(ctx, invoke(ctx, argc, argv, std::make_index_sequence<num_args>{}));
        }

        /**
         * @internal
         * @brief Call the function with the provided arguments and return the return value as a JSValue.
         * @param ctx Current JavaScript context.
         * @param argc Argument count.
         * @param argv Argument list.
         * @return JSValue representing the return value of the function.
         */
        template <typename TRetI = TRet>
        static std::enable_if_t<std::is_same_v<TRetI, void>, JSValue> call_impl(
            JSContext *ctx, int argc, JSValue *argv) {
            invoke(ctx, argc, argv, std::make_index_sequence<num_args>{});
            return arg_list_helpers::set<undefined>(ctx, undefined{});
        }
    };

    using inner = binder_inner<decltype(Func)>;
    static constexpr size_t num_args = inner::num_args;
    /**
     * @brief Thunk for calling `Func` from JavaScript.
     * @param ctx Current JavaScript context.
     * @param argc Argument count.
     * @param argv Argument list.
     * @return JSValue representing the return value of the function, or an exception JSValue if an error occurs.
     */
    HEDLEY_NON_NULL(1, 4)
    static JSValue call(JSContext *ctx, JSValue, int argc, JSValue *argv) {
        try {
            return inner::call_impl(ctx, argc, argv);
        } catch (js_exception &e) {
            return e.v;
        }
    }

    /**
     * @brief Thunk for calling `Func` as a constructor from JavaScript.
     * @param ctx Current JavaScript context.
     * @param func_obj JSValue representing the function object (not used).
     * @param js_this JSValue representing the `this` context (should be a new object).
     * @param argc Argument count.
     * @param argv Argument list.
     * @param flags Flags for the call (not used).
     * @return JSValue representing the return value of the function, or an exception JSValue if an error occurs.
     */
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

/**
 * @brief Binder for a class member function.
 * @tparam Klass Class the function belongs to.
 * @tparam Func Function pointer to bind.
 */
template <typename Klass, auto Func> struct class_binder {
    /**
     * @internal
     * @brief Forward declaration of the inner binder type, to allow destructuring of the function signature.
     * @tparam T Type of the function
     */
    template <typename T> struct binder_inner;
    /**
     * @internal
     * @brief Binder for a function pointer type.
     * @tparam TRet Return type of the function.
     * @tparam TArgs Argument types of the function.
     */
    template <typename TRet, typename... TArgs> struct binder_inner<TRet (Klass::*)(TArgs...)> {
        using ret_type = getter_type_t<TRet>;
        using arg_types = std::tuple<TArgs...>;
        static constexpr size_t num_args = sizeof...(TArgs);

        /**
         * @internal
         * @brief Invoke `Klass->Func` with the provided arguments.
         * @tparam Is Indices of the arguments to pass to the function.
         * @param ctx Current JavaScript context.
         * @param js_this JavaScript `this` object, which is expected to be an instance of `Klass`.
         * @param argc Argument count.
         * @param argv Argument list.
         * @return Return value of `Klass->Func`.
         */
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
