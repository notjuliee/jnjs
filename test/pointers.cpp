#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

using namespace jnjs;

namespace {

struct point {
    int value = 0;

    int get_value() { return value; }
    void set_value(int v) { value = v; }

    constexpr static wrapped_class_builder<point> build_js_class() {
        wrapped_class_builder<point> builder("point");
        builder.bind_getset<&point::get_value, &point::set_value>("value");
        return builder;
    }
};

int point_value(const point *p) { return p ? p->value : -1; }
point *forward_point(point *p) { return p; }

} // namespace

TEST_CASE("Pointer bindings", "[pointer]") {
    auto ctx = runtime::new_context();
    ctx.install_class<point>();

    point origin{41};
    point spare{12};

    ctx.set_global("origin", &origin);
    ctx.set_global("spare", &spare);
    ctx.set_global_fn<point_value>("pointValue");
    ctx.set_global_fn<forward_point>("forwardPoint");

    SECTION("Receives pointer from JS object") {
        REQUIRE(ctx.eval("pointValue(origin)") == origin.value);
        REQUIRE(ctx.eval("pointValue(spare)") == spare.value);
        REQUIRE(ctx.eval("origin.value = 99; origin.value;") == 99);
        REQUIRE(origin.value == 99);
    }

    SECTION("Null pointer handling") {
        REQUIRE(ctx.eval("pointValue()") == -1);
        REQUIRE(ctx.eval("pointValue(null)") == -1);
    }

    SECTION("Pointer roundtrip") {
        auto ret = ctx.eval("forwardPoint(origin)");
        auto *ptr = ret.as<point *>();
        REQUIRE(ptr == &origin);
        REQUIRE(ctx.eval("forwardPoint(origin).value") == origin.value);
    }
}
