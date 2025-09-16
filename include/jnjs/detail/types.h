#pragma once

namespace jnjs {

namespace detail {
struct internal_class_meta_data {
    uint32_t id = 0;
};
template <typename T> struct internal_class_meta {
    static inline internal_class_meta_data data = {};
};

template <typename T, typename = void> constexpr bool has_build_v = false;
} // namespace detail

struct undefined {
    constexpr bool operator==(const undefined &) const { return true; }
};
struct null {
    constexpr bool operator==(const null &) const { return true; }
};
template <typename T> struct must_be {
    must_be() = default;
    must_be(T v) : v(v) {}
    operator T() const { return v; }

    const bool operator==(const must_be &o) { return v == o.v; }
    const bool operator==(const T &o) { return v == o; }

    T v;
};

} // namespace jnjs