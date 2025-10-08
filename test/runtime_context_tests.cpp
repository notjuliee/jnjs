#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>
#include <jnjs/detail/js_storage.h>
#include <jnjs/detail/types.h>

#include "helpers.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct sample_bound {
    explicit sample_bound(int v = 0) : data(v) {}

    int get() const { return data; }
    void set(int v) { data = v; }
    int add(int delta) const { return data + delta; }

    constexpr static jnjs::wrapped_class_builder<sample_bound> build_js_class() {
        jnjs::wrapped_class_builder<sample_bound> builder("SampleBound");
        builder.bind_function<&sample_bound::get>("get");
        builder.bind_function<&sample_bound::set>("set");
        builder.bind_function<&sample_bound::add>("add");
        return builder;
    }

    int data = 0;
};

struct reinstallable {
    explicit reinstallable(int v = 0) : data(v) {}

    int get_value() const { return data; }
    void set_value(int v) { data = v; }

    constexpr static jnjs::wrapped_class_builder<reinstallable> build_js_class() {
        jnjs::wrapped_class_builder<reinstallable> builder("Reinstallable");
        builder.bind_ctor<int>();
        builder.bind_function<&reinstallable::get_value>("getValue");
        builder.bind_function<&reinstallable::set_value>("setValue");
        return builder;
    }

    int data = 0;
};

} // namespace

TEST_CASE("Contexts created from a shared runtime keep globals isolated", "[runtime][context]") {
    auto ctx_a = jnjs::runtime::new_context();
    auto ctx_b = jnjs::runtime::new_context();

    ctx_a.eval("globalThis.marker = 123;");

    auto in_a = ctx_a.eval("marker");
    auto in_b = ctx_b.eval("typeof marker");

    REQUIRE(in_a == 123);
    REQUIRE(in_b == std::string{"undefined"});
}

TEST_CASE("Destroying a context releases QuickJS state", "[runtime][context]") {
    {
        auto ctx = jnjs::runtime::new_context();
        ctx.eval("globalThis.ephemeral = 'value';");
    } // ctx is destroyed here

    auto fresh_ctx = jnjs::runtime::new_context();
    auto result = fresh_ctx.eval("typeof ephemeral");

    REQUIRE(result == std::string{"undefined"});
}

TEST_CASE("set_global round-trips supported types", "[runtime][context][set_global]") {
    auto ctx = jnjs::runtime::new_context();
    ctx.install_class<sample_bound>();

    SECTION("primitive values") {
        ctx.set_global("num", 42);
        ctx.set_global("pi", 3.5);
        ctx.set_global("greeting", std::string{"hello"});

        REQUIRE(ctx.eval("num") == 42);
        REQUIRE(ctx.eval("pi").kinda_eq(3.5));
        REQUIRE(ctx.eval("greeting") == std::string{"hello"});
    }

    SECTION("STL containers") {
        ctx.set_global("numbers", std::vector<int>{1, 2, 3});
        ctx.set_global("mapping", std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}});

        auto array_check = ctx.eval("Array.isArray(numbers)");
        REQUIRE(array_check == true);

        auto doubled = ctx.eval("numbers.map(n => n * 2)");
        auto doubled_vec = doubled.as<std::vector<int>>();
        REQUIRE(doubled_vec == std::vector<int>{2, 4, 6});

        auto obj = ctx.eval("mapping");
        auto as_map = obj.as<std::unordered_map<std::string, int>>();
        REQUIRE(as_map.at("a") == 1);
        REQUIRE(as_map.at("b") == 2);
    }

    SECTION("pointer types via class bindings") {
        sample_bound instance{7};
        ctx.set_global("instance", &instance);

        ctx.eval("instance.set(12);");
        REQUIRE(instance.data == 12);

        auto sum = ctx.eval("instance.add(5)");
        REQUIRE(sum == 17);

        auto round_trip = ctx.eval("instance").as<sample_bound *>();
        REQUIRE(round_trip == &instance);
    }
}

TEST_CASE("Globals with unusual identifiers remain addressable via bracket syntax", "[runtime][context][set_global]") {
    auto ctx = jnjs::runtime::new_context();

    ctx.set_global("", 1);
    ctx.set_global("123name", 2);
    ctx.set_global("with-hyphen", 3);

    auto empty_key = ctx.eval("Object.prototype.hasOwnProperty.call(globalThis, '')");
    REQUIRE(empty_key == true);

    auto leading_digit = ctx.eval("globalThis['123name']");
    REQUIRE(leading_digit == 2);

    auto hyphen = ctx.eval("globalThis['with-hyphen']");
    REQUIRE(hyphen == 3);

    // Accessing invalid identifiers directly is a syntax error; QuickJS surfaces this as an exception value.
    auto syntax_error = ctx.eval("(() => { try { return eval('123name'); } catch (err) { return err.name; } })()");
    REQUIRE(syntax_error == std::string{"SyntaxError"});
}

TEST_CASE("install_class shares QuickJS metadata across contexts", "[runtime][class]") {
    auto &meta = jnjs::detail::internal_class_meta<jnjs::detail::stored_class<reinstallable>>::data;
    meta.id = 0;

    auto ctx_a = jnjs::runtime::new_context();
    auto ctx_b = jnjs::runtime::new_context();

    ctx_a.install_class<reinstallable>();
    const auto first_id = meta.id;
    REQUIRE(first_id != 0);

    const std::vector<std::string> expected_keys{"constructor", "getValue", "setValue"};
    auto check_proto = [&](jnjs::context &ctx) {
        auto keys = ctx.eval("Reflect.ownKeys(Reinstallable.prototype)").as<std::vector<std::string>>();
        std::sort(keys.begin(), keys.end());
        REQUIRE(keys == expected_keys);
    };

    check_proto(ctx_a);
    REQUIRE(ctx_a.eval("(() => { const inst = new Reinstallable(4); inst.setValue(9); return inst.getValue(); })()") == 9);

    ctx_a.install_class<reinstallable>();
    REQUIRE(meta.id == first_id);
    check_proto(ctx_a);

    ctx_b.install_class<reinstallable>();
    REQUIRE(meta.id == first_id);
    check_proto(ctx_b);
    REQUIRE(ctx_b.eval("(() => { const other = new Reinstallable(7); return other.getValue(); })()") == 7);
}
