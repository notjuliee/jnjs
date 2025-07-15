#pragma once

#include <memory>
#include <string_view>

#include "binding.h"
#include "value.h"

#include "detail/function_helpers.h"
#include "detail/quickjs_stub.h"

namespace jnjs {

class context : detail::qjs::impl_ptr<detail::qjs::JSContext> {
    using base = detail::qjs::impl_ptr<detail::qjs::JSContext>;

  public:
    explicit context(detail::qjs::JSRuntime &);
    ~context() = default;

    value eval(std::string_view code);

    void set_global(const char *name, const value &v);

    template <typename T> value make_value(const T &) const;
    template <typename K> value make_static_value(K *v) const {
        return _make_class_value(detail::internal_class_meta<K>::data.id, v);
    }
    template <auto Func> value make_cfunc_value(const char *name) const {
        return _make_cfunc_value(name, detail::binder<Func>::call, detail::binder<Func>::num_args);
    }

    template <typename K, typename = std::enable_if_t<detail::has_build_v<K>, void>> void install_class() {
        constexpr static wrapped_class_builder<K> binder = K::build();
        return _decl_class_impl(binder._d, detail::internal_class_meta<K>::data);
    }

  private:
    value _make_class_value(uint32_t id, void *ptr) const;
    value _make_cfunc_value(const char *name, void *fn, int len) const;
    void _decl_class_impl(const detail::class_builder_data &, detail::internal_class_meta_data &o);
};

} // namespace jnjs