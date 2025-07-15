#include <quickjs.h>

#include <jnjs/runtime.h>

#include <jnjs/context.h>

namespace jnjs {

namespace {} // namespace

runtime::runtime() : base(JS_NewRuntime(), JS_FreeRuntime) {}

context runtime::_new_context() { return context(*get()); }

} // namespace jnjs