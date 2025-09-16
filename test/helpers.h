#pragma once

#include <ostream>

#include <jnjs/value.h>

namespace jnjs {
inline std::ostream &operator<<(std::ostream &os, const value &v) {
    os << v.as<std::string>();
    return os;
}
inline std::ostream &operator<<(std::ostream &os, const undefined &) {
    os << "undefined";
    return os;
}
} // namespace jnjs
