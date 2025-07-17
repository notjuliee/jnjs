#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

namespace {
int c_add(int a, int b) { return a + b; }
} // namespace

TEST_CASE("Basic functions", "[functions]") {
    auto ctx = jnjs::runtime::new_context();
    jnjs::function f1;
    {
        REQUIRE(ctx.eval("function add_js(a, b) { return a + b; }").is<jnjs::undefined>());
        auto add_js = ctx.eval("add_js");
        REQUIRE(add_js.is<jnjs::function>());
        f1 = add_js.as<jnjs::function>();
    }

    SECTION("call") {
        auto r = f1(1, 2);
        REQUIRE(r.is<int>());
        REQUIRE(r.as<int>() == 3);
    }

    {
        ctx.set_global_fn<c_add>("c_add");
        REQUIRE(ctx.eval("c_add").is<jnjs::function>());
    }

    jnjs::function f2;
    {
        auto v2 = ctx.eval("(a, b) => { return c_add(a, b); }");
        REQUIRE(v2.is<jnjs::function>());
        f2 = v2.as<jnjs::function>();
    }

    SECTION("call_layer") {
        REQUIRE(ctx.eval("c_add(1, 2)").is<int>());
        auto r = f2(1, 2);
        REQUIRE(r.is<int>());
        REQUIRE(r.as<int>() == 3);
    }
}