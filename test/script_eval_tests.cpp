#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

#include <string>
#include <vector>

TEST_CASE("context::eval returns strongly typed results", "[eval]") {
    auto ctx = jnjs::runtime::new_context();

    auto sum = ctx.eval("1 + 2");
    REQUIRE(sum == 3);

    auto obj = ctx.eval("({ a: 41, b: 'value', nested: { flag: true } })");
    REQUIRE(obj["a"] == 41);
    REQUIRE(obj["b"] == std::string{"value"});
    REQUIRE(obj["nested"]["flag"] == true);

    auto arr = ctx.eval("[0, 1, 2, 3]");
    REQUIRE(arr[2] == 2);

    auto vec = arr.as<std::vector<int>>();
    REQUIRE(vec == std::vector<int>{0, 1, 2, 3});
}

TEST_CASE("context::eval treats empty programs as undefined", "[eval]") {
    auto ctx = jnjs::runtime::new_context();

    auto blank = ctx.eval("   \n\t   ");
    REQUIRE(blank.is<jnjs::undefined>());
}

TEST_CASE("Multi-line programs execute without truncation", "[eval]") {
    auto ctx = jnjs::runtime::new_context();
    const char *script = R"(
        (() => {
            let total = 0;
            for (let i = 0; i <= 100; ++i) {
                total += i;
            }
            return { total, size: 101 };
        })()
    )";

    auto result = ctx.eval(script);
    REQUIRE(result["total"] == 5050);
    REQUIRE(result["size"] == 101);
}

TEST_CASE("Syntax errors surface QuickJS exceptions", "[eval][errors]") {
    auto ctx = jnjs::runtime::new_context();
    auto faulty = ctx.eval("const x = ;");

    auto message = faulty.as<std::string>();
    REQUIRE(message.find("SyntaxError") != std::string::npos);
}

TEST_CASE("Runtime errors propagate back to callers", "[eval][errors]") {
    auto ctx = jnjs::runtime::new_context();
    auto thrown = ctx.eval("(() => { throw new TypeError('nope'); })()");

    auto message = thrown.as<std::string>();
    REQUIRE(message.find("TypeError") != std::string::npos);
    REQUIRE(message.find("nope") != std::string::npos);
}
