#include <catch2/catch_all.hpp>

#include <jnjs/jnjs.h>

using namespace jnjs;

namespace {

struct static_test {
    int do_something() { return 42; }

    value call_js_function(function fn, const value &v) { return fn(v); }

    constexpr static wrapped_class_builder<static_test> build() {
        wrapped_class_builder<static_test> builder("static_test");
        builder.bind_function<&static_test::do_something>("do_something");
        builder.bind_function<&static_test::call_js_function>("call_js_function");
        return builder;
    }
};

struct dynamic_test {
    explicit dynamic_test(std::optional<int> a_) : a(a_.value_or(0)) {}

    int get() { return a; }
    void set(int a_) { a = a_; }
    void copy(const dynamic_test &o) { a = o.a; }

    constexpr static wrapped_class_builder<dynamic_test> build() {
        wrapped_class_builder<dynamic_test> builder("dynamic_test");
        builder.bind_ctor<std::optional<int>>();
        builder.bind_getset<&dynamic_test::get, &dynamic_test::set>("a");
        builder.bind_function<&dynamic_test::copy>("copy");
        return builder;
    }

    int a;
};

} // namespace

TEST_CASE("Class binding", "[class]") {
    auto ctx = runtime::new_context();
    ctx.install_class<static_test>();
    ctx.install_class<dynamic_test>();
    static_test st;
    ctx.set_global("st", ctx.make_static_value(&st));

    SECTION("Something works") {
        auto ret = ctx.eval("st.do_something()");
        REQUIRE(ret.is<int>());
        REQUIRE(ret.as<int>() == st.do_something());
    }

    SECTION("Function calling") {
        auto ret = ctx.eval("st.call_js_function((a) => a + 1, 1)");
        auto es = ret.as<std::string>();
        REQUIRE(ret.is<int>());
        REQUIRE(ret.as<int>() == 2);
    }

    SECTION("Dynamic creation single") {
        REQUIRE(ctx.eval("const i = new dynamic_test();").is<undefined>());
        REQUIRE(ctx.eval("i.a").as<int>() == 0);
        REQUIRE(ctx.eval("i.a = 3").as<int>() == 3);
        REQUIRE(ctx.eval("i.a").as<int>() == 3);
        REQUIRE(ctx.eval("const i2 = new dynamic_test(42);").is<undefined>());
        REQUIRE(ctx.eval("i2.a").as<int>() == 42);
        REQUIRE(ctx.eval("i.copy(i2)").is<undefined>());
        REQUIRE(ctx.eval("i.a").as<int>() == 42);
    }
}