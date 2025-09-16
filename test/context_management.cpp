#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

using namespace jnjs;

namespace {

int global_counter = 0;

void increment_counter() { ++global_counter; }
int get_counter() { return global_counter; }
void reset_counter() { global_counter = 0; }

} // namespace

TEST_CASE("Context isolation", "[context][isolation]") {
    reset_counter();

    SECTION("Multiple independent contexts") {
        auto ctx1 = runtime::new_context();
        auto ctx2 = runtime::new_context();

        // Set different globals in each context
        ctx1.set_global("value", 10);
        ctx2.set_global("value", 20);

        // Contexts should be isolated
        REQUIRE(ctx1.eval("value") == 10);
        REQUIRE(ctx2.eval("value") == 20);

        // Modify in one context
        ctx1.eval("value = 15;");

        // Other context should be unaffected
        REQUIRE(ctx1.eval("value") == 15);
        REQUIRE(ctx2.eval("value") == 20);
    }

    SECTION("Function binding isolation") {
        auto ctx1 = runtime::new_context();
        auto ctx2 = runtime::new_context();

        // Bind function to only one context
        ctx1.set_global_fn<increment_counter>("increment");
        ctx1.set_global_fn<get_counter>("getCounter");

        // Function should work in ctx1
        ctx1.eval("increment();");
        REQUIRE(ctx1.eval("getCounter()") == 1);

        // Function should not exist in ctx2
        auto result = ctx2.eval("typeof increment");
        REQUIRE(result.as<std::string>() == "undefined");
    }

    SECTION("Variable scoping across contexts") {
        auto ctx1 = runtime::new_context();
        auto ctx2 = runtime::new_context();

        // Define function in ctx1
        ctx1.eval("function testFunc() { return 'ctx1'; }");

        // Define same-named function in ctx2
        ctx2.eval("function testFunc() { return 'ctx2'; }");

        // Each should return its own version
        REQUIRE(ctx1.eval("testFunc()").as<std::string>() == "ctx1");
        REQUIRE(ctx2.eval("testFunc()").as<std::string>() == "ctx2");
    }
}

TEST_CASE("Global object manipulation", "[context][globals]") {
    auto ctx = runtime::new_context();

    SECTION("Setting and getting globals") {
        // Basic types
        ctx.set_global("intValue", 42);
        ctx.set_global("stringValue", std::string("hello"));
        ctx.set_global("boolValue", true);

        REQUIRE(ctx.eval("intValue") == 42);
        REQUIRE(ctx.eval("stringValue").as<std::string>() == "hello");
        REQUIRE(ctx.eval("boolValue") == true);

        // Complex types
        std::vector<int> vec = {1, 2, 3, 4};
        ctx.set_global("arrayValue", vec);

        auto result = ctx.eval("arrayValue");
        REQUIRE(result.is<std::vector<int>>());
        REQUIRE(result.as<std::vector<int>>() == vec);
    }

    SECTION("Overwriting globals") {
        ctx.set_global("changingValue", 100);
        REQUIRE(ctx.eval("changingValue") == 100);

        // Overwrite with different type
        ctx.set_global("changingValue", std::string("now a string"));
        REQUIRE(ctx.eval("changingValue").as<std::string>() == "now a string");

        // Overwrite with same type
        ctx.set_global("changingValue", std::string("updated string"));
        REQUIRE(ctx.eval("changingValue").as<std::string>() == "updated string");
    }

    SECTION("Invalid global names") {
        // These might be handled differently by the implementation
        ctx.set_global("", 42);           // Empty name
        ctx.set_global("123invalid", 42); // Starts with number
        ctx.set_global("with-dash", 42);  // Contains dash

        // Test if they can be accessed (behavior may vary)
        auto result1 = ctx.eval("typeof window['']");
        auto result2 = ctx.eval("typeof window['123invalid']");
        auto result3 = ctx.eval("typeof window['with-dash']");
    }
}

TEST_CASE("Context evaluation edge cases", "[context][eval]") {
    auto ctx = runtime::new_context();

    SECTION("Syntax errors") {
        // Invalid JavaScript syntax
        auto result1 = ctx.eval("var x = ;"); // Incomplete statement
        // Should handle syntax error gracefully

        auto result2 = ctx.eval("function() {}"); // Missing name
        // Should handle syntax error gracefully

        auto result3 = ctx.eval("1 + + 2"); // Invalid operator sequence
        // Should handle syntax error gracefully
    }

    SECTION("Runtime errors") {
        // Reference error
        auto result1 = ctx.eval("undefinedVariable.property");
        // Should handle runtime error gracefully

        // Type error
        auto result2 = ctx.eval("null.someMethod()");
        // Should handle runtime error gracefully

        // Custom error
        auto result3 = ctx.eval("throw new Error('Custom error');");
        // Should handle thrown error gracefully
    }

    SECTION("Empty and whitespace evaluation") {
        auto result1 = ctx.eval("");
        REQUIRE(result1 == undefined{});

        auto result2 = ctx.eval("   ");
        REQUIRE(result2 == undefined{});

        auto result3 = ctx.eval("\n\t  \n");
        REQUIRE(result3 == undefined{});
    }

    SECTION("Very long code evaluation") {
        // Generate a long but valid JavaScript expression
        std::string long_expr = "1";
        for (int i = 0; i < 1000; ++i) {
            long_expr += " + 1";
        }

        auto result = ctx.eval(long_expr);
        REQUIRE(result == 1001);
    }

    SECTION("Multi-line evaluation") {
        std::string multi_line = R"(
            var a = 5;
            var b = 10;
            function calculate() {
                return a * b + 2;
            }
            calculate();
        )";

        auto result = ctx.eval(multi_line);
        REQUIRE(result == 52);
    }
}

TEST_CASE("Context memory and cleanup", "[context][memory]") {
    SECTION("Context destruction") {
        // Create context in limited scope
        {
            auto ctx = runtime::new_context();
            ctx.set_global("testValue", 12345);
            REQUIRE(ctx.eval("testValue") == 12345);
        } // Context should be destroyed here

        // Create new context - should not have previous values
        auto new_ctx = runtime::new_context();
        auto result = new_ctx.eval("typeof testValue");
        REQUIRE(result.as<std::string>() == "undefined");
    }

    SECTION("Large object handling") {
        auto ctx = runtime::new_context();

        // Create large array in JavaScript
        ctx.eval("var largeArray = new Array(10000).fill(42);");

        auto result = ctx.eval("largeArray.length");
        REQUIRE(result == 10000);

        auto first_elem = ctx.eval("largeArray[0]");
        REQUIRE(first_elem == 42);

        auto last_elem = ctx.eval("largeArray[9999]");
        REQUIRE(last_elem == 42);
    }

    SECTION("Circular reference handling") {
        auto ctx = runtime::new_context();

        // Create circular reference
        ctx.eval(R"(
            var obj1 = { name: 'obj1' };
            var obj2 = { name: 'obj2' };
            obj1.ref = obj2;
            obj2.ref = obj1;
        )");

        // Should handle circular references without issues
        REQUIRE(ctx.eval("obj1.name").as<std::string>() == "obj1");
        REQUIRE(ctx.eval("obj2.name").as<std::string>() == "obj2");
        REQUIRE(ctx.eval("obj1.ref.name").as<std::string>() == "obj2");
        REQUIRE(ctx.eval("obj2.ref.name").as<std::string>() == "obj1");
    }
}