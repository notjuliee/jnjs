#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace {

int add(int a, int b) { return a + b; }

int sum_variadic(int base, jnjs::remaining_args<int> rest) {
    int total = base;
    for (int v : rest) {
        total += v;
    }
    return total;
}

int optional_sum(int base, std::optional<int> maybe) { return base + maybe.value_or(5); }

std::vector<int> double_values(std::vector<int> values) {
    for (auto &v : values) {
        v *= 2;
    }
    return values;
}

int map_size(std::unordered_map<std::string, int> values) { return static_cast<int>(values.size()); }

bool g_mark_called = false;
void mark_called() { g_mark_called = true; }

int may_throw(int value) {
    if (value < 0) {
        throw std::runtime_error("negative value");
    }
    return value;
}

struct bound_point {
    explicit bound_point(int v = 0) : value(v) {}

    int get() const { return value; }
    void increment(int delta) { value += delta; }

    constexpr static jnjs::wrapped_class_builder<bound_point> build_js_class() {
        jnjs::wrapped_class_builder<bound_point> builder("BoundPoint");
        builder.bind_function<&bound_point::get>("get");
        builder.bind_function<&bound_point::increment>("increment");
        return builder;
    }

    int value = 0;
};

int read_point(bound_point *pt) { return pt ? pt->value : -1; }

} // namespace

TEST_CASE("set_global_fn exposes free functions with accurate signatures", "[binding][function]") {
    auto ctx = jnjs::runtime::new_context();

    ctx.set_global_fn<&add>("add");
    ctx.set_global_fn<&sum_variadic>("sum_variadic");
    ctx.set_global_fn<&optional_sum>("optional_sum");
    ctx.set_global_fn<&double_values>("double_values");
    ctx.set_global_fn<&map_size>("map_size");
    ctx.set_global_fn<&mark_called>("mark_called");
    ctx.set_global_fn<&may_throw>("may_throw");

    REQUIRE(ctx.eval("add(2, 3)") == 5);
    REQUIRE(ctx.eval("sum_variadic(1, 2, 3, 4)") == 10);
    REQUIRE(ctx.eval("optional_sum(4)") == 9);
    REQUIRE(ctx.eval("optional_sum(4, null)") == 9);
    REQUIRE(ctx.eval("optional_sum(4, 6)") == 10);

    auto doubled = ctx.eval("double_values([1, 2, 3])").as<std::vector<int>>();
    REQUIRE(doubled == std::vector<int>{2, 4, 6});

    auto map_count = ctx.eval("map_size({ a: 1, b: 2 })");
    REQUIRE(map_count == 2);

    g_mark_called = false;
    auto void_result = ctx.eval("mark_called()");
    REQUIRE(void_result.is<jnjs::undefined>());
    REQUIRE(g_mark_called);
}

TEST_CASE("Bound functions translate exceptions into JavaScript errors", "[binding][function][errors]") {
    auto ctx = jnjs::runtime::new_context();
    ctx.set_global_fn<&may_throw>("may_throw");

    auto message = ctx.eval("(() => { try { may_throw(-1); return 'ok'; } catch (err) { return err.message; } })()");
    REQUIRE(message == std::string{"C++ exception: negative value"});
}

TEST_CASE("Bound functions handle class pointers and references", "[binding][class]") {
    auto ctx = jnjs::runtime::new_context();
    ctx.install_class<bound_point>();
    ctx.set_global_fn<&read_point>("read_point");

    bound_point point{5};
    ctx.set_global("point", &point);

    REQUIRE(ctx.eval("read_point(point)") == 5);
    ctx.eval("point.increment(7);");
    REQUIRE(point.value == 12);
    REQUIRE(ctx.eval("read_point(point)") == 12);
    REQUIRE(ctx.eval("read_point()") == -1);
    REQUIRE(ctx.eval("read_point(null)") == -1);
    REQUIRE(ctx.eval("read_point(42)") == -1);
    REQUIRE(ctx.eval("read_point({ nope: true })") == -1);
}

TEST_CASE("jnjs::function instances remain callable after copies and moves", "[function][interop]") {
    auto ctx = jnjs::runtime::new_context();

    auto fn_value = ctx.eval("(a, b) => ({ sum: a + b })");
    auto fn = fn_value.as<jnjs::function>();

    auto copy = fn;
    auto moved = std::move(fn);

    auto call_result = copy(5, 7);
    REQUIRE(call_result["sum"] == 12);

    auto again = moved(2, 3);
    REQUIRE(again["sum"] == 5);

    auto thrown = ctx.eval("(() => { throw new Error('boom'); })").as<jnjs::function>();
    auto error_value = thrown();
    auto message = error_value.as<std::string>();
    REQUIRE(message.find("Error: boom") != std::string::npos);
}

TEST_CASE("jnjs::function preserves functions capturing `this`", "[function][interop][this]") {
    auto ctx = jnjs::runtime::new_context();

    auto fn = ctx.eval(R"(
        (() => {
            const obj = {
                base: 10,
                makeAdder(step) {
                    return addend => this.base + step + addend;
                }
            };
            const adder = obj.makeAdder(3);
            obj.base = 20;
            return adder;
        })()
    )")
                    .as<jnjs::function>();

    REQUIRE(fn(7) == 30);
}
