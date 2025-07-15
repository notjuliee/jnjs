#include <quickjs.h>

#include <jnjs/value.h>

#include "value_helpers.h"

namespace jnjs {

value::value() : _v(JS_UNDEFINED), _ctx(nullptr) {}
value::~value() {
    if (_ctx != nullptr) {
        JS_FreeValue(_ctx, _v);
    }
    _ctx = nullptr;
}

value &value::operator=(const value &o) noexcept {
    if (this != &o) {
        if (o._ctx != nullptr) {
            _v = JS_DupValue(o._ctx, o._v);
            _ctx = o._ctx;
        } else {
            _v = o._v;
            _ctx = nullptr;
        }
    }
    return *this;
}
value &value::operator=(value &&o) noexcept {
    if (this != &o) {
        _v = o._v;
        _ctx = o._ctx;
        o._v = JS_UNDEFINED;
        o._ctx = nullptr;
    }
    return *this;
}

bool value::operator==(const value &o) const {
    const JSValue v1 = _v;
    const JSValue v2 = o._v;
    if (v1.tag == JS_TAG_UNDEFINED && v2.tag == JS_TAG_UNDEFINED)
        return true;
    if (v1.tag == JS_TAG_NULL && v2.tag == JS_TAG_NULL)
        return true;
    if (v1.tag == JS_TAG_INT && v2.tag == JS_TAG_INT)
        return v1.u.int32 == v2.u.int32;
}

#define X(T)                                                                                                           \
    template <> bool value::is<T>() const { return detail::value_helpers<T>::is(_ctx, _v); }                           \
    template <> bool value::is_convertible<T>() const { return detail::value_helpers<T>::is_convertible(_ctx, _v); }   \
    template <> T value::as<T>() const { return detail::value_helpers<T>::as(_ctx, _v); }

#include "all_types.inl"

#undef X

} // namespace jnjs
