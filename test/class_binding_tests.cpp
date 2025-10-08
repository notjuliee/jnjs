#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

#include <optional>

namespace {

struct utility_holder {
    int do_something() { return 42; }

    jnjs::value call_js_function(jnjs::function fn, const jnjs::value &v) { return fn(v); }

    constexpr static jnjs::wrapped_class_builder<utility_holder> build_js_class() {
        jnjs::wrapped_class_builder<utility_holder> builder("UtilityHolder");
        builder.bind_function<&utility_holder::do_something>("doSomething");
        builder.bind_function<&utility_holder::call_js_function>("callJsFunction");
        return builder;
    }
};

struct configurable_point {
    explicit configurable_point(std::optional<int> initial) : value(initial.value_or(0)) {}

    int get() const { return value; }
    void set(int next) { value = next; }
    void copy_from(const configurable_point &other) { value = other.value; }

    constexpr static jnjs::wrapped_class_builder<configurable_point> build_js_class() {
        jnjs::wrapped_class_builder<configurable_point> builder("ConfigurablePoint");
        builder.bind_ctor<std::optional<int>>();
        builder.bind_getset<&configurable_point::get, &configurable_point::set>("value");
        builder.bind_function<&configurable_point::copy_from>("copyFrom");
        return builder;
    }

    int value = 0;
};

} // namespace

TEST_CASE("Class bindings expose methods and interop with JavaScript callbacks", "[binding][class]") {
    auto ctx = jnjs::runtime::new_context();
    ctx.install_class<utility_holder>();

    utility_holder holder;
    ctx.set_global("holder", &holder);

    auto result = ctx.eval("holder.doSomething()");
    REQUIRE(result == 42);

    auto callback_result = ctx.eval("holder.callJsFunction(x => x + 1, 5)");
    REQUIRE(callback_result == 6);
}

TEST_CASE("Class constructors, getters, and setters behave as expected", "[binding][class][accessors]") {
    auto ctx = jnjs::runtime::new_context();
    ctx.install_class<configurable_point>();

    ctx.eval("globalThis.pt = new ConfigurablePoint();");
    REQUIRE(ctx.eval("pt.value") == 0);
    REQUIRE(ctx.eval("pt.value = 7") == 7);
    REQUIRE(ctx.eval("pt.value") == 7);

    ctx.eval("globalThis.other = new ConfigurablePoint(13);");
    REQUIRE(ctx.eval("other.value") == 13);
    REQUIRE(ctx.eval("pt.copyFrom(other); pt.value;") == 13);
}

TEST_CASE("Native code can interact with JavaScript-created instances", "[binding][class][interop]") {
    auto ctx = jnjs::runtime::new_context();
    ctx.install_class<configurable_point>();

    auto value = ctx.eval("new ConfigurablePoint(9)");
    REQUIRE(value.is<configurable_point *>());

    auto *pointer = value.as<configurable_point *>();
    REQUIRE(pointer != nullptr);
    REQUIRE(pointer->value == 9);

    ctx.set_global("fromNative", pointer);
    REQUIRE(ctx.eval("fromNative.value = 21") == 21);
    REQUIRE(pointer->value == 21);

    pointer->value = 33;
    REQUIRE(ctx.eval("fromNative.value") == 33);
}
