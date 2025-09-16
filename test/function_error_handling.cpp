#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

using namespace jnjs;

namespace {

// Test functions with various signatures
int simple_add(int a, int b) { return a + b; }

int throws_exception(int x) {
    if (x < 0) {
        throw std::runtime_error("Negative input not allowed");
    }
    return x * 2;
}

void void_function() {}

std::string string_concat(const std::string &a, const std::string &b) { return a + b; }

// Function with variadic args
int sum_variadic(const remaining_args<int> &args) {
    int sum = 0;
    for (int val : args) {
        sum += val;
    }
    return sum;
}

// Function expecting specific container type
int sum_vector(const std::vector<int> &vec) {
    int sum = 0;
    for (int val : vec) {
        sum += val;
    }
    return sum;
}

int call_and_add_1(function js_fn, int x) {
    auto result = js_fn(x);
    return result.as<int>() + 1;
}

std::vector<std::string> complex_func(int x) {
    return {"item" + std::to_string(x), "another" + std::to_string(x * 2)};
};

} // namespace

TEST_CASE("Function parameter validation", "[function][error]") {
    auto ctx = runtime::new_context();
    ctx.set_global_fn<simple_add>("simpleAdd");
    ctx.set_global_fn<string_concat>("stringConcat");
    ctx.set_global_fn<sum_variadic>("sumVariadic");
    ctx.set_global_fn<sum_vector>("sumVector");

    SECTION("Wrong argument count") {
        // Too few arguments - should handle gracefully
        auto result1 = ctx.eval("simpleAdd(5)"); // missing second argument
        // Behavior may vary - could be undefined, throw, or use default

        // Too many arguments - should ignore extras
        auto result2 = ctx.eval("simpleAdd(1, 2, 3, 4)");
        REQUIRE(result2 == 3); // Should still work with first two args
    }

    SECTION("Type mismatches") {
        // String passed to int parameter
        auto result1 = ctx.eval("simpleAdd('hello', 'world')");
        // Should attempt conversion or handle gracefully

        // Number passed to string parameter
        auto result2 = ctx.eval("stringConcat(123, 456)");
        REQUIRE(result2.is<std::string>());
        REQUIRE(result2.as<std::string>() == "123456");
    }

    SECTION("Variadic argument edge cases") {
        // No arguments to variadic function
        auto result1 = ctx.eval("sumVariadic()");
        REQUIRE(result1 == 0);

        // Mixed types in variadic args
        auto result2 = ctx.eval("sumVariadic(1, 2.5, 3)");
        REQUIRE(result2 == 6); // 2.5 should convert to 2

        // Non-numeric types in variadic args
        auto result3 = ctx.eval("sumVariadic(1, 'invalid', 3)");
        // Should handle conversion or skip invalid args
    }

    SECTION("Container parameter validation") {
        // Valid array
        auto result1 = ctx.eval("sumVector([1, 2, 3, 4])");
        REQUIRE(result1 == 10);

        // Empty array
        auto result2 = ctx.eval("sumVector([])");
        REQUIRE(result2 == 0);

        // Mixed type array
        auto result3 = ctx.eval("sumVector([1, 'two', 3])");
        // Should handle type conversion or filtering

        // Non-array passed to vector parameter
        auto result4 = ctx.eval("sumVector('not an array')");
        // Should handle gracefully, possibly return 0 or throw
    }
}

TEST_CASE("Function exception handling", "[function][exception]") {
    auto ctx = runtime::new_context();
    ctx.set_global_fn<throws_exception>("throwsException");
    ctx.set_global_fn<void_function>("voidFunction");

    SECTION("C++ exceptions in bound functions") {
        // Valid input - should work
        auto result1 = ctx.eval("throwsException(5)");
        REQUIRE(result1 == 10);

        // Invalid input that causes an exception. Catch it in JS to avoid leaving
        // an unhandled exception live in the QuickJS context, which can trigger
        // a GC assert at JS_FreeRuntime.
        auto caught =
            ctx.eval("(() => { try { throwsException(-1); return 'nope'; } catch (e) { return 'caught'; } })()");
        REQUIRE(caught == std::string("caught"));
    }

    SECTION("Void function return handling") {
        auto result = ctx.eval("voidFunction()");
        REQUIRE(result.is<undefined>());
    }
}

TEST_CASE("Function call edge cases", "[function][edge]") {
    auto ctx = runtime::new_context();

    SECTION("Calling non-function values") {
        ctx.set_global("notAFunction", 42);

        // Attempting to call a non-function should be handled gracefully
        // This tests the function call mechanism itself
        auto js_func = ctx.eval("notAFunction");
        REQUIRE_FALSE(js_func.is<function>());
    }

    SECTION("Function with complex return types") {
        ctx.set_global_fn<complex_func>("complexFunc");

        auto result = ctx.eval("complexFunc(3)");
        REQUIRE(result.is<std::vector<std::string>>());
        auto vec = result.as<std::vector<std::string>>();
        REQUIRE(vec.size() == 2);
        REQUIRE(vec[0] == "item3");
        REQUIRE(vec[1] == "another6");
    }

    SECTION("Recursive function calls") {
        ctx.set_global_fn<call_and_add_1>("callbackFunc");

        // JS function that calls the C++ function
        ctx.eval("function recursive(x) { return x <= 0 ? 0 : callbackFunc(recursive, x - 1); }");

        auto result = ctx.eval("recursive(3)");
        REQUIRE(result.is<int>());
        // Should handle the recursive call chain properly
    }
}