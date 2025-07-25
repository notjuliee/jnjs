#pragma once
/**
 * @file runtime.h
 * @brief JS runtime.
 */

#include <memory>

#include "context.h"

#include "detail/util.h"

namespace jnjs {

/**
 * @brief JS runtime instance.
 * @note Only one runtime can ever exist during the applications lifecycle.
 */
class runtime : detail::impl_ptr<JSRuntime> {
    using base = detail::impl_ptr<JSRuntime>;

  public:
    /**
     * @brief Get the singleton instance of the JS runtime.
     * @return Singleton instance of the JS runtime.
     */
    static runtime &instance() {
        static runtime instance;
        return instance;
    }

    /**
     * @brief Create a new js context.
     * @return New JS context using this runtime.
     */
    static context new_context() { return instance()._new_context(); }

  private:
    runtime();            /**< @internal Create a new runtime. */
    ~runtime() = default; /**< @internal Destroy the runtime. */

    context _new_context(); /**< @internal Create a new JS context using this runtime. */
};

} // namespace jnjs