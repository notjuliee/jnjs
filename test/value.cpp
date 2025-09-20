#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

TEST_CASE("Value semantics", "[value]") {
    auto ctx = jnjs::runtime::new_context();

    SECTION("Copy retains reference") {
        auto shared = ctx.eval("globalThis.shared = { count: 1 }; shared;");
        auto copy = shared;
        REQUIRE(shared["count"] == 1);
        REQUIRE(copy["count"] == 1);
        REQUIRE(ctx.eval("shared.count = 5; shared.count;") == 5);
        REQUIRE(copy["count"] == 5);
    }

    SECTION("Move transfers ownership") {
        auto original = ctx.eval("({ flag: 1 })");
        auto moved = std::move(original);
        REQUIRE(moved["flag"] == 1);
        REQUIRE(original["flag"].is<jnjs::undefined>());
    }

    SECTION("Global exposure shares state") {
        auto holder = ctx.eval("({ value: 2 })");
        ctx.set_global("holder", holder);
        REQUIRE(ctx.eval("holder.value") == 2);
        REQUIRE(ctx.eval("holder.value = 9; holder.value;") == 9);
        REQUIRE(holder["value"] == 9);
    }
}

