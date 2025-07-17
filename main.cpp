#include <cassert>
#include <iostream>

#include <jnjs/jnjs.h>

int do_something(const std::string &v) { return (int)v.length(); }

struct test_class {
    void do_something(int a1, const std::optional<int> a2 = std::nullopt) {
        std::cout << "A1 = " << a1 << ", A2 = " << a2.value_or(42) << std::endl;
    }
    void do_something_else(std::optional<jnjs::must_be<int>> a1) {
        std::cout << "A1 = " << a1.value_or(42) << std::endl;
    }

    int add(int a1, std::optional<int> a2 = std::nullopt) { return a1 + a2.value_or(1); }

    constexpr static jnjs::wrapped_class_builder<test_class> build_js_class() {
        jnjs::wrapped_class_builder<test_class> b("test_class");
        b.bind_function<&test_class::do_something>("do_something");
        b.bind_function<&test_class::do_something_else>("do_something_else");
        b.bind_function<&test_class::add>("add");
        return b;
    }
};

struct class_2 {
    JNJS_IMPL_NON_COPYABLE(class_2);

    explicit class_2(const std::string &greet) : _greet(greet) { std::cout << "class2(" << _greet << ")" << std::endl; }
    ~class_2() { std::cout << "~class2(" << _greet << ")" << std::endl; }
    std::string _greet;
    void hi(const std::string &who) { std::cout << _greet << ", " << who << std::endl; }

    void hi2(const class_2 *other) {
        std::cout << _greet << ", " << (other ? other->_greet : std::string("???")) << std::endl;
    }

    constexpr static jnjs::wrapped_class_builder<class_2> build_js_class() {
        jnjs::wrapped_class_builder<class_2> b("class_2");
        b.bind_ctor<const std::string &>();
        b.bind_function<&class_2::hi>("hi");
        b.bind_function<&class_2::hi2>("hi2");
        return b;
    }
};

namespace {
jnjs::value eval_log(jnjs::context &ctx, const char *code) {
    auto r = ctx.eval(code);
    if (!r.is<jnjs::undefined>()) {
        std::cout << r.as<std::string>() << std::endl;
    }
    return r;
}
} // namespace

int main() {
    auto ctx = jnjs::runtime::new_context();
    ctx.install_class<test_class>();
    ctx.install_class<class_2>();

    {
        auto fn = eval_log(ctx, "(a, b) => { return a + b }").as<jnjs::function>();
        auto ret = fn(1, 2);

        test_class t;
        ctx.set_global("test_class", &t);

        eval_log(ctx, "test_class.do_something('123')");
        eval_log(ctx, "test_class.do_something(123, 456)");
        eval_log(ctx, "test_class.do_something_else()");
        eval_log(ctx, "test_class.do_something_else('123')");
        eval_log(ctx, "test_class.add(1, 2)");
        eval_log(ctx, "test_class.add(1, null)");

        eval_log(ctx, "const c1 = new class_2('waow')");
        eval_log(ctx, "const c2 = new class_2('hi')");
        eval_log(ctx, "c1.hi('loser'); c2.hi('cool');");
        eval_log(ctx, "c1.hi2(c2); c1.hi2(); c2.hi2(null);");
    }
    return 0;
}