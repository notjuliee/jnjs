#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include <numeric>

#include "helpers.h"

using namespace jnjs;

namespace {
int get_answer() { return 42; }

int sum_all(const remaining_args<int> &args) { return std::accumulate(args.begin(), args.end(), 0); }

int sum_all_plus(int a1, const remaining_args<int> &args) { return a1 + std::accumulate(args.begin(), args.end(), 0); }

int sum_all_array(const std::vector<int> &args) { return std::accumulate(args.begin(), args.end(), 0); }

std::unordered_map<std::string, int> increment_key(
    const std::unordered_map<std::string, int> &map, const std::string &key) {
    auto out = map;
    out[key] += 1;
    return out;
}

} // namespace

TEST_CASE("Function binding", "[function]") {
    auto ctx = runtime::new_context();
    ctx.set_global_fn<get_answer>("getAnswer");
    ctx.set_global_fn<sum_all>("sumAll");
    ctx.set_global_fn<sum_all_plus>("sumAllPlus");
    ctx.set_global_fn<sum_all_array>("sumAllArray");
    ctx.set_global_fn<increment_key>("incrementKey");
    REQUIRE(ctx.eval("getAnswer()") == 42);
    REQUIRE(ctx.eval("sumAll(1, 2, 3)") == 6);
    REQUIRE(ctx.eval("sumAllPlus(10, 1, 2, 3)") == 16);
    REQUIRE(ctx.eval("incrementKey({a: 1, b: 2}, 'a').a") == 2);
    REQUIRE(ctx.eval("incrementKey({a: 1, b: 2}, 'b').b") == 3);
    REQUIRE(ctx.eval("sumAllArray([1, 2, 3])") == 6);
}
