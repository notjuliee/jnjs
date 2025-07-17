#pragma once

/**
 * @file jnjs.h
 * @brief all includes for jnjs
 * @date 2025-07-14
 */

#include <memory>

#include "context.h"

#include "detail/util.h"

namespace jnjs {

/**
 * @brief js runtime instance
 * @note only one runtime can ever exist in the applications lifecycle
 */
class runtime : detail::impl_ptr<JSRuntime> {
    using base = detail::impl_ptr<JSRuntime>;

  public:
    static runtime &instance() {
        static runtime instance;
        return instance;
    }

    /**
     * @brief create a new js context
     * @return a js context using this runtime
     */
    static context new_context() { return instance()._new_context(); }

  private:
    runtime();
    ~runtime() = default;

    context _new_context();
};

} // namespace jnjs