#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

using namespace jnjs;

TEST_CASE("Value type checking edge cases", "[value][edge]") {
    auto ctx = runtime::new_context();

    SECTION("Type checking with invalid types") {
        auto null_val = ctx.eval("null");
        auto undefined_val = ctx.eval("undefined");
        auto nan_val = ctx.eval("NaN");
        auto inf_val = ctx.eval("Infinity");

        // Test is<T>() with null/undefined
        REQUIRE_FALSE(null_val.is<int>());
        REQUIRE_FALSE(null_val.is<std::string>());
        REQUIRE_FALSE(undefined_val.is<int>());
        REQUIRE_FALSE(undefined_val.is<std::string>());

        // Test special numeric values
        REQUIRE(nan_val.is<double>());
        REQUIRE(inf_val.is<double>());
        REQUIRE_FALSE(nan_val.is<int>());
        REQUIRE_FALSE(inf_val.is<int>());
    }

    SECTION("Numeric conversion edge cases") {
        // Integer overflow cases
        auto large_num = ctx.eval("9007199254740992"); // 2^53, beyond safe int range
        REQUIRE(large_num.is<double>());

        // NaN and Infinity handling
        auto nan_val = ctx.eval("NaN");
        auto inf_val = ctx.eval("Infinity");
        auto neg_inf_val = ctx.eval("-Infinity");

        REQUIRE(std::isnan(nan_val.as<double>()));
        REQUIRE(std::isinf(inf_val.as<double>()));
        REQUIRE(std::isinf(neg_inf_val.as<double>()));
        REQUIRE(neg_inf_val.as<double>() < 0);
    }

    SECTION("String conversion edge cases") {
        // Empty string
        auto empty_str = ctx.eval("''");
        REQUIRE(empty_str.is<std::string>());
        REQUIRE(empty_str.as<std::string>() == "");

        // Unicode strings
        auto unicode_str = ctx.eval("'Hello 🌍'");
        REQUIRE(unicode_str.is<std::string>());
        REQUIRE(unicode_str.as<std::string>() == "Hello 🌍");

        // Very long string (test memory handling)
        auto long_str = ctx.eval("'x'.repeat(10000)");
        REQUIRE(long_str.is<std::string>());
        REQUIRE(long_str.as<std::string>().length() == 10000);
    }

    SECTION("Container conversion edge cases") {
        // Empty containers
        auto empty_array = ctx.eval("[]");
        auto empty_obj = ctx.eval("{}");

        REQUIRE(empty_array.is<std::vector<int>>());
        REQUIRE(empty_array.as<std::vector<int>>().empty());

        REQUIRE(empty_obj.as<std::unordered_map<std::string, int>>().empty());

        // Nested containers
        auto nested_array = ctx.eval("[[1, 2], [3, 4]]");
        REQUIRE(nested_array.is<std::vector<std::vector<int>>>());
        auto result = nested_array.as<std::vector<std::vector<int>>>();
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == std::vector<int>{1, 2});
        REQUIRE(result[1] == std::vector<int>{3, 4});
    }
}

TEST_CASE("Value conversion error handling", "[value][error]") {
    auto ctx = runtime::new_context();

    SECTION("Invalid as<T>() conversions") {
        auto str_val = ctx.eval("'not a number'");
        auto obj_val = ctx.eval("{}");
        auto array_val = ctx.eval("[1, 2, 3]");

        // These should not throw but return default/invalid values
        // The exact behavior depends on implementation
        REQUIRE_FALSE(str_val.is<int>());
        REQUIRE_FALSE(obj_val.is<int>());
        REQUIRE_FALSE(array_val.is<int>());
    }

    SECTION("Array access bounds") {
        auto array_val = ctx.eval("[1, 2, 3]");

        // Valid access
        REQUIRE(array_val[0] == 1);
        REQUIRE(array_val[2] == 3);

        // Out of bounds access should return undefined
        REQUIRE(array_val[10].is<undefined>());
        REQUIRE(array_val[-1].is<undefined>());
    }

    SECTION("Object property access") {
        auto obj_val = ctx.eval("({a: 1, b: 2})");

        // Valid access
        REQUIRE(obj_val["a"] == 1);
        REQUIRE(obj_val["b"] == 2);

        // Non-existent property should return undefined
        REQUIRE(obj_val["nonexistent"].is<undefined>());
        REQUIRE(obj_val[""].is<undefined>());
    }
}