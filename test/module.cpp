#include <catch2/catch_test_macros.hpp>

#include <jnjs/jnjs.h>

using namespace jnjs;

namespace {

class test_class {
  public:
    test_class() = default;

    void set_v(JSValue nv) { v = nv; }
    JSValue get_v() { return v; }

    JSValue v = JS_UNDEFINED;

    constexpr static wrapped_class_builder<test_class> build_js_class() {
        wrapped_class_builder<test_class> b("test_class");
        b.bind_ctor<>();
        b.bind_getset<&test_class::get_v, &test_class::set_v>("v");
        return b;
    }
};

} // namespace

TEST_CASE("Module", "[module]") { auto ctx = runtime::new_context(); }