#pragma once

#include <cstring>
#include <memory>

namespace jnjs::detail::qjs {
struct JSValue {
    JSValue() = default;
    uint64_t _1 = 0;
    uint64_t _2 = 0;
#ifdef QUICKJS_H
    JSValue(::JSValue o) { std::memcpy(this, &o, sizeof(JSValue)); }
    operator ::JSValue() const {
        ::JSValue ret;
        std::memcpy(&ret, this, sizeof(JSValue));
        return ret;
    }
#endif
};
#ifdef QUICKJS_H
static_assert(sizeof(JSValue) == sizeof(::JSValue), "JSValue Size Matches");
using JSContext = JSContext;
using JSRuntime = JSRuntime;
#else
struct JSContext;
struct JSRuntime;
#endif
using fn_generic = JSValue (*)(JSContext *, JSValue, int, JSValue *);
using fn_getter = JSValue (*)(JSContext *, JSValue);
using fn_setter = JSValue (*)(JSContext *, JSValue, JSValue);
using fn_dtor = void (*)(JSContext *, JSValue);
using fn_call = JSValue (*)(JSContext *, JSValue, JSValue, int argc, JSValue *argv, int flags);
union fn_type {
    fn_generic generic;
};
enum class fn_type_which : uint8_t {
    generic = 0,
    generic_magic = 1,
    constructor = 2,
};
enum class fn_prop_flags : uint8_t {
    configurable = 1 << 0,
    writable = 1 << 1,
    enumerable = 1 << 2,
};
enum class fn_def_type : uint8_t {
    function = 0,
    getset = 1,
};
struct JSCFunctionListEntry {
    const char *name;
    fn_prop_flags prop_flags;
    fn_def_type def_type;
    uint16_t magic;
    union {
        struct {
            uint8_t length;
            fn_type_which cproto;
            fn_type fn;
        } func;
        struct {
            fn_getter getter;
            fn_setter setter;
        } getset;
    };
};
struct JSClassDef {
    const char *class_name = nullptr;
    fn_dtor dtor = nullptr;
    void *gc_mark = nullptr;
    fn_call call = nullptr;
    void *exotic = nullptr;
};
#ifdef QUICKJS_H
static_assert(sizeof(fn_type) == sizeof(::JSCFunctionType));
static_assert(sizeof(JSCFunctionListEntry) == sizeof(::JSCFunctionListEntry));
static_assert(sizeof(JSClassDef) == sizeof(::JSClassDef));
#endif
template <typename T> using delete_fn = void (*)(T *);
template <typename T> using impl_ptr = std::unique_ptr<T, delete_fn<T>>;
} // namespace jnjs::detail::qjs
