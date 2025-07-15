#pragma once

#include <memory>

#include "context.h"

namespace jnjs {

class runtime : detail::qjs::impl_ptr<detail::qjs::JSRuntime> {
    using base = detail::qjs::impl_ptr<detail::qjs::JSRuntime>;

  public:
    static runtime &instance() {
        static runtime instance;
        return instance;
    }

    static context new_context() { return instance()._new_context(); }

  private:
    runtime();
    ~runtime() = default;

    context _new_context();
};

} // namespace jnjs