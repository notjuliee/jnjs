#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>

TEST_CASE("value copy shares handles while move transfers ownership", "[value][semantics]") {
    auto ctx = jnjs::runtime::new_context();

    auto original = ctx.eval("({ count: 0 })");
    ctx.set_global("sharedRef", original);

    auto copy = original;
    ctx.eval("sharedRef.count = 21;");

    REQUIRE(copy["count"] == 21);

    auto moved = std::move(original);
    REQUIRE(moved["count"] == 21);
    REQUIRE(original.is<jnjs::undefined>());
}

TEST_CASE("value::operator[] handles key varieties", "[value][indexing]") {
    auto ctx = jnjs::runtime::new_context();
    auto data = ctx.eval(R"(
        (() => {
            const sym = Symbol.for('key');
            const obj = { label: 'jnjs', values: [10, 20, 30] };
            obj[sym] = 99;
            return { obj, sym };
        })()
    )");

    auto obj = data["obj"];
    auto sym = data["sym"];

    REQUIRE(obj["label"] == std::string{"jnjs"});
    REQUIRE(obj["values"][1] == 20);
    REQUIRE(obj[sym] == 99);
}

TEST_CASE("Type conversion helpers cover common numeric and container cases", "[value][conversion]") {
    auto ctx = jnjs::runtime::new_context();
    auto values = ctx.eval(R"(
        ({
            boolTrue: true,
            boolFalse: false,
            intVal: 42,
            doubleVal: 3.5,
            nanVal: NaN,
            infVal: Infinity,
            text: 'hello',
            emptyVec: [],
            nestedVec: [[1, 2], [3, 4]],
            map: { first: 1, second: 2 }
        })
    )");

    REQUIRE(values["boolTrue"] == true);
    REQUIRE(values["boolFalse"] == false);
    REQUIRE(values["intVal"] == 42);
    REQUIRE(values["doubleVal"].kinda_eq(3.5));

    auto nan_val = values["nanVal"].as<double>();
    REQUIRE(std::isnan(nan_val));

    auto inf_val = values["infVal"].as<double>();
    REQUIRE(std::isinf(inf_val));

    REQUIRE(values["text"] == std::string{"hello"});

    auto empty_vec = values["emptyVec"].as<std::vector<int>>();
    REQUIRE(empty_vec.empty());

    auto nested_vec = values["nestedVec"].as<std::vector<std::vector<int>>>();
    REQUIRE(nested_vec == std::vector<std::vector<int>>{{1, 2}, {3, 4}});

    auto map = values["map"].as<std::unordered_map<std::string, int>>();
    REQUIRE(map.at("first") == 1);
    REQUIRE(map.at("second") == 2);
}

TEST_CASE("is<T>, is_convertible<T>, and kinda_eq highlight conversion rules", "[value][conversion]") {
    auto ctx = jnjs::runtime::new_context();

    auto num = ctx.eval("21");
    auto str_num = ctx.eval("'21'");

    REQUIRE(num.is<int>());
    REQUIRE(num.is_convertible<double>());

    REQUIRE_FALSE(str_num.is<int>());
    REQUIRE(str_num.is_convertible<int>());
    REQUIRE(str_num.kinda_eq(21));

    auto not_array = ctx.eval("42");
    REQUIRE_FALSE(not_array.is<std::vector<int>>());
    REQUIRE_FALSE(not_array.is_convertible<std::vector<int>>());
}

TEST_CASE("Container conversions surface type mismatches", "[value][conversion][errors]") {
    auto ctx = jnjs::runtime::new_context();

    auto bad_vector = ctx.eval("[Symbol('oops')]");
    auto coerced_vector = bad_vector.as<std::vector<std::string>>();
    REQUIRE(coerced_vector.size() == 1);
    REQUIRE(coerced_vector.front().find("error") != std::string::npos);

    auto bad_map = ctx.eval("({ key: Symbol('value') })");
    auto coerced_map = bad_map.as<std::unordered_map<std::string, std::string>>();
    REQUIRE(coerced_map.at("key").find("error") != std::string::npos);
}
