#pragma once

#include "detail/fwd.h"
#include "detail/quickjs_stub.h"

namespace jnjs {

class value {
  public:
    value();
    ~value();

    value(const value &o) { *this = o; }
    value(value &&o) noexcept { *this = std::forward<value>(o); }
    value &operator=(const value &) noexcept;
    value &operator=(value &&o) noexcept;

    bool operator==(const value &o) const;

    template <typename T> bool is() const;
    template <typename T> bool is_convertible() const { return is<T>(); }
    template <typename T> T as() const;

  private:
    explicit value(const detail::qjs::JSValue v, detail::qjs::JSContext *ctx) : _v(v), _ctx(ctx) {
        //
        return;
    }
    detail::qjs::JSValue _v = {};
    detail::qjs::JSContext *_ctx = nullptr;
    friend context;
    friend function;
    friend detail::impl_value_helpers;
};

} // namespace jnjs