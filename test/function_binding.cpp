#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

using namespace jnjs;

namespace {
int get_answer() { return 42; }
} // namespace

TEST_CASE("Function binding", "[function]") {
    auto ctx = runtime::new_context();
    ctx.set_global_fn<get_answer>("getAnswer");
    REQUIRE(ctx.eval("getAnswer()").as<int>() == 42);
}
