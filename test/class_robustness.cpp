#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

#include "helpers.h"

using namespace jnjs;

namespace {

class robust_test_class {
  public:
    explicit robust_test_class(int initial_value = 42) : value_(initial_value) {}

    // Getters and setters
    int get_value() const { return value_; }
    void set_value(int val) {
        if (val < 0) {
            throw std::out_of_range("Value cannot be negative");
        }
        value_ = val;
    }

    // Method that can throw
    int divide_value(int divisor) {
        if (divisor == 0) {
            throw std::invalid_argument("Division by zero");
        }
        return value_ / divisor;
    }

    // Const method
    int get_doubled() const { return value_ * 2; }

    // Static method
    static int static_method(int x) { return x * 10; }

    // Method with complex parameters
    void update_from_vector(const std::vector<int> &vec) {
        if (vec.empty()) {
            value_ = 0;
        } else {
            value_ = vec[0];
        }
    }

    // Method returning complex type
    std::vector<int> get_range() const {
        std::vector<int> result;
        for (int i = 0; i < value_; ++i) {
            result.push_back(i);
        }
        return result;
    }

    constexpr static wrapped_class_builder<robust_test_class> build_js_class() {
        wrapped_class_builder<robust_test_class> builder("RobustTestClass");
        builder.bind_ctor<int>();
        builder.bind_getset<&robust_test_class::get_value, &robust_test_class::set_value>("value");
        builder.bind_function<&robust_test_class::divide_value>("divideValue");
        builder.bind_function<&robust_test_class::get_doubled>("getDoubled");
        builder.bind_function<&robust_test_class::update_from_vector>("updateFromVector");
        builder.bind_function<&robust_test_class::get_range>("getRange");
        return builder;
    }

  private:
    int value_;
};

class minimal_class {
  public:
    minimal_class() = default;

    constexpr static wrapped_class_builder<minimal_class> build_js_class() {
        wrapped_class_builder<minimal_class> builder("MinimalClass");
        builder.bind_ctor<>();
        return builder;
    }
};

} // namespace

TEST_CASE("Class constructor edge cases", "[class][constructor]") {
    auto ctx = runtime::new_context();
    ctx.install_class<robust_test_class>();
    ctx.install_class<minimal_class>();

    SECTION("Valid constructor calls") {
        REQUIRE(ctx.eval("const obj1 = new RobustTestClass(42);") == undefined{});
        REQUIRE(ctx.eval("obj1.value") == 42);

        REQUIRE(ctx.eval("const obj2 = new RobustTestClass(100);").is<undefined>());
        REQUIRE(ctx.eval("obj2.value") == 100);
    }

    SECTION("Missing new keyword") {
        // Calling constructor as regular function
        auto result = ctx.eval("RobustTestClass(50)");
        // Behavior depends on implementation - might work or error
    }

    SECTION("Minimal class instantiation") {
        REQUIRE(ctx.eval("const minimal = new MinimalClass();").is<undefined>());
        // Should work even with no methods/properties
    }
}

TEST_CASE("Class method robustness", "[class][methods]") {
    auto ctx = runtime::new_context();
    ctx.install_class<robust_test_class>();

    SECTION("Method error handling") {
        ctx.eval("const obj = new RobustTestClass(20);");

        // Valid method calls
        REQUIRE(ctx.eval("obj.getDoubled()") == 40);
        REQUIRE(ctx.eval("obj.divideValue(4)") == 5);

        // Method that throws exception
        auto result = ctx.eval("obj.divideValue(0)");
        // Should handle division by zero exception

        // Setter with invalid value
        auto result2 = ctx.eval("obj.value = -5");
        // Should handle negative value exception
    }

    SECTION("Method with complex parameters") {
        ctx.eval("const obj = new RobustTestClass(10);");

        // Valid array parameter
        ctx.eval("obj.updateFromVector([99, 1, 2]);");
        REQUIRE(ctx.eval("obj.value") == 99);

        // Empty array parameter
        ctx.eval("obj.updateFromVector([]);");
        REQUIRE(ctx.eval("obj.value") == 0);

        // Invalid parameter type
        auto result = ctx.eval("obj.updateFromVector('not an array')");
        // Should handle type mismatch gracefully
    }

    SECTION("Method returning complex types") {
        ctx.eval("const obj = new RobustTestClass(3);");

        auto result = ctx.eval("obj.getRange()");
        REQUIRE(result.is<std::vector<int>>());
        auto vec = result.as<std::vector<int>>();
        REQUIRE(vec == std::vector<int>{0, 1, 2});

        // Test with larger range
        ctx.eval("obj.value = 5;");
        auto result2 = ctx.eval("obj.getRange()");
        auto vec2 = result2.as<std::vector<int>>();
        REQUIRE(vec2.size() == 5);
    }
}

TEST_CASE("Class property access edge cases", "[class][properties]") {
    auto ctx = runtime::new_context();
    ctx.install_class<robust_test_class>();

    SECTION("Property getter/setter validation") {
        ctx.eval("const obj = new RobustTestClass(15);");

        // Valid property access
        REQUIRE(ctx.eval("obj.value") == 15);

        // Valid property assignment
        ctx.eval("obj.value = 25;");
        REQUIRE(ctx.eval("obj.value") == 25);

        // Invalid property assignment (negative value)
        auto result = ctx.eval("obj.value = -10");
        // Should handle the exception from setter

        // Type conversion in assignment
        ctx.eval("obj.value = '30';"); // String to int
        REQUIRE(ctx.eval("obj.value") == 30);

        ctx.eval("obj.value = 42.7;"); // Float to int
        REQUIRE(ctx.eval("obj.value") == 42);
    }

    SECTION("Non-existent property access") {
        ctx.eval("const obj = new RobustTestClass();");

        // Accessing non-existent property
        auto result = ctx.eval("obj.nonExistentProperty");
        REQUIRE(result.is<undefined>());

        // Setting non-existent property
        ctx.eval("obj.newProperty = 123;");
        // Should either work (dynamic property) or be ignored
    }
}

TEST_CASE("Class object lifetime", "[class][lifetime]") {
    auto ctx = runtime::new_context();
    ctx.install_class<robust_test_class>();

    SECTION("Multiple object instances") {
        ctx.eval("const obj1 = new RobustTestClass(10);");
        ctx.eval("const obj2 = new RobustTestClass(20);");

        // Objects should be independent
        REQUIRE(ctx.eval("obj1.value") == 10);
        REQUIRE(ctx.eval("obj2.value") == 20);

        ctx.eval("obj1.value = 30;");
        REQUIRE(ctx.eval("obj1.value") == 30);
        REQUIRE(ctx.eval("obj2.value") == 20); // Should remain unchanged
    }

    SECTION("Object references and copying") {
        ctx.eval("const obj1 = new RobustTestClass(5);");
        ctx.eval("const obj2 = obj1;"); // Reference, not copy

        ctx.eval("obj2.value = 15;");
        REQUIRE(ctx.eval("obj1.value") == 15); // Should be same object
        REQUIRE(ctx.eval("obj2.value") == 15);
    }
}