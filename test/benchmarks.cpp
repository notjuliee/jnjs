#include <catch2/catch_all.hpp>

#include <jnjs/jnjs.h>

#ifdef _MSC_VER
#define noinline __declspec(noinline)
#else
#define noinline __attribute__((noinline))
#endif

namespace {
noinline volatile int c_add(volatile int a, volatile int b) { return a + b; }
} // namespace

TEST_CASE("Function benchmarks", "[benchmarks]") {
    auto ctx = jnjs::runtime::new_context();
    ctx.set_global("c_add", ctx.make_cfunc_value<c_add>("c_add"));
    auto f_js = ctx.eval("(a, b) => { return a + b; }").as<jnjs::function>();
    auto f_c = ctx.eval("c_add").as<jnjs::function>();
    auto f_js_c = ctx.eval("(a, b) => { return c_add(a, b); }").as<jnjs::function>();
    auto f_js_c_n =
        ctx.eval("(count) => { let sum = 0; for (let i = 0; i < count; i++) { sum += c_add(i, i); } return sum; }")
            .as<jnjs::function>();

    auto iter_count = GENERATE(1, 1000);

    BENCHMARK("add_c iters=" + std::to_string(iter_count)) {
        uint64_t sum = 0;
        for (int i = 0; i < iter_count; ++i) {
            sum += c_add(i, i);
        }
        return sum;
    };
    BENCHMARK("add_f_js iters=" + std::to_string(iter_count)) {
        uint64_t sum = 0;
        for (int i = 0; i < iter_count; ++i) {
            sum += f_js(i, i).as<int>();
        }
        return sum;
    };
    BENCHMARK("add_f_c iters=" + std::to_string(iter_count)) {
        uint64_t sum = 0;
        for (int i = 0; i < iter_count; ++i) {
            sum += f_c(i, i).as<int>();
        }
    };
    BENCHMARK("add_f_js_c iters=" + std::to_string(iter_count)) {
        uint64_t sum = 0;
        for (int i = 0; i < iter_count; ++i) {
            sum += f_js_c(i, i).as<int>();
        }
        return sum;
    };
    BENCHMARK("add_f_js_c_n iters=" + std::to_string(iter_count)) { return f_js_c_n(iter_count).as<int>(); };
}