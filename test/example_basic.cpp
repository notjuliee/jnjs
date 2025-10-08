#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

// Example test showing the preferred Catch2 layout.
TEST_CASE("Evaluating simple expressions returns typed results", "[example]") {
    auto ctx = jnjs::runtime::new_context();

    SECTION("1 + 1 evaluates to 2") {
        REQUIRE(ctx.eval("1 + 1") == 2);
    }
}
