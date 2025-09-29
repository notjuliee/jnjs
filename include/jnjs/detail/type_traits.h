#pragma once

#include <type_traits>

#include "fwd.h"
#include "js_storage.h"
#include "types.h"

namespace jnjs::detail {

template <typename T>
constexpr bool has_build_v<T, std::void_t<decltype(&T::build_js_class)>> =
    std::is_same_v<decltype(&T::build_js_class), wrapped_class_builder<T> (*)()>;

template <typename T> struct getter_type {
    using type = std::remove_cvref_t<T>;
};

template <typename T> struct getter_type<stored_class<T> *> {
    using type = stored_class<T> *;
};

template <typename T> using getter_type_t = getter_type<T>::type;

template <typename T> struct remove_member_const {
    using type = T;
};
template <typename Klass, typename TRet, typename... TArgs>
struct remove_member_const<TRet (Klass::*)(TArgs...) const> {
    using type = TRet (Klass::*)(TArgs...);
};
template <typename T> using remove_member_const_t = typename remove_member_const<T>::type;

} // namespace jnjs::detail
