#pragma once

namespace jnjs {

namespace detail {
struct internal_class_meta_data {
    void *ctx = nullptr;
    uint32_t id = 0;
};
template <typename T> struct internal_class_meta {
    static inline internal_class_meta_data data = {};
};

struct impl_wrapped_class {
    void *ptr;
    uint32_t id;

    template <typename T> constexpr static impl_wrapped_class from(T *t) {
        return {t, internal_class_meta<T>::data.id};
    }
};

template <typename T, typename = void> constexpr bool has_build_v = false;
} // namespace detail

struct undefined {};
struct null {};
template <typename T> struct must_be {
    must_be() = default;
    must_be(T v) : v(v) {}
    operator T() const { return v; }

    T v;
};
template <typename T> struct is_class {
    is_class(T &v_) : v(v_);
    operator T &() const { return v; }

    T &v;
};

} // namespace jnjs