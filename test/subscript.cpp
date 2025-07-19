#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

TEST_CASE("Subscript operator", "[subscript]") {
    auto ctx = jnjs::runtime::new_context();
    jnjs::value v1 = ctx.eval("({ a: 1, b: 2 })");
    jnjs::value v2 = ctx.eval("[2, 3]");

    SECTION("Get property by string") {
        REQUIRE(v1["a"] == 1);
        REQUIRE(v1["b"] == 2);
    }

    SECTION("Get property by index") {
        REQUIRE(v1[0].is<jnjs::undefined>());
        REQUIRE(v1[1].is<jnjs::undefined>());
        REQUIRE(v2[0] == 2);
        REQUIRE(v2[1] == 3);
    }
}