#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

TEST_CASE("Basic tests", "[basic]") {
    auto ctx = jnjs::runtime::new_context();
    SECTION("eval") {
        jnjs::value v;
        REQUIRE_NOTHROW(v = ctx.eval("1 + 1"));
        REQUIRE(v.is<int>());
        REQUIRE(v.as<int>() == 2);
    }
}
