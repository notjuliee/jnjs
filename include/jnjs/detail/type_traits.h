#pragma once

#include <type_traits>

#include "fwd.h"
#include "types.h"

namespace jnjs::detail {

template <typename T> using remove_ref_cv_t = std::remove_reference_t<std::remove_cv_t<T>>;

#if 0
template <typename T>
concept has_build = requires(T) {
    { T::build_js_class() } -> std::same_as<wrapped_class_builder<T>>;
};
#endif

template <typename T>
constexpr bool has_build_v<T, std::void_t<decltype(&T::build_js_class)>> =
    std::is_same_v<decltype(&T::build_js_class), wrapped_class_builder<T> (*)()>;

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

} // namespace jnjs::detail
