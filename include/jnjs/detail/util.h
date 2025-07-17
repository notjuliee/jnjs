#pragma once

#include <memory>

#define JNJS_IMPL_NON_COPYABLE(klass)                                                                                  \
    klass(const klass &) = delete;                                                                                     \
    klass &operator=(const klass &) = delete;
#define JNJS_IMPL_NON_MOVABLE(klass)                                                                                   \
    klass(klass &&) = delete;                                                                                          \
    klass &operator=(klass &&) = delete;
#define JNJS_IMPL_MOVABLE_IMPL_DEFN(klass)                                                                             \
    klass(klass &&) = default;                                                                                         \
    klass &operator=(klass &&o) noexcept
#define JNJS_IMPL_NON_COPYABLE_MOVABLE(klass)                                                                          \
    JNJS_IMPL_NON_COPYABLE(klass);                                                                                     \
    JNJS_IMPL_NON_MOVABLE(klass);
#define JNJS_IMPL_MOVABLE_DEFAULT_I(klass)                                                                             \
    JNJS_IMPL_MOVABLE_IMPL_DEFN(klass) {                                                                               \
        _i = std::move(o._i);                                                                                          \
        return *this;                                                                                                  \
    }

namespace jnjs::detail {

template <typename T> struct key {
  private:
    constexpr key() = default;
    friend T;
};

template<typename T>
using impl_ptr = std::unique_ptr<T, void (*)(T *)>;

} // namespace jnjs::detail