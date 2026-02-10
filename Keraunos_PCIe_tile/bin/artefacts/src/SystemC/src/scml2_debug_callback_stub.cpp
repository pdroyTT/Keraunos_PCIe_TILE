// Stub implementation for Virtualizer SystemC debug callback registry
// This is needed because Virtualizer's SystemC headers reference debug callback symbols
// that aren't in standard SystemC libraries

#include <sysc/kernel/snps_sc_debug_callback.h>

namespace snps {
namespace sc {

// Provide stub implementation for debug callback registry static member
debug_callback_registry* debug_callback_registry::m_instance = nullptr;

// Provide stub implementation for debug callback registry
void debug_callback_registry::invoke_callbacks(debug_callback_list*& callbacks,
                                                sc_core::sc_object const* obj,
                                                sc_core::sc_event const* ev) {
    // Empty stub - debug callbacks not required for basic functionality
    (void)callbacks;
    (void)obj;
    (void)ev;
}

} // namespace sc
} // namespace snps
