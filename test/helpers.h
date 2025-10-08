#pragma once

#include <catch2/catch_tostring.hpp>

#include <jnjs/jnjs.h>

namespace Catch {

template <> struct StringMaker<jnjs::undefined> {
    static std::string convert(const jnjs::undefined &) { return "undefined"; }
};

template <> struct StringMaker<jnjs::value> {
    static std::string convert(const jnjs::value &v) {
        if (v == jnjs::undefined{}) {
            return "undefined";
        }
        if (v == jnjs::null{}) {
            return "null";
        }
        try {
            return v.as<std::string>();
        } catch (const std::exception &e) {
            return std::string{"<non-string value: "} + e.what() + ">";
        } catch (...) {
            return "<non-string value>";
        }
    }
};

} // namespace Catch
