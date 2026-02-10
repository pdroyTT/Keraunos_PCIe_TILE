// Stub implementation for Virtualizer SystemC module extension
// This is needed because Virtualizer's SystemC headers reference end_of_construction()
// that may not be in standard SystemC libraries

#include <sysc/kernel/sc_module.h>

namespace sc_core {

// Provide stub implementation for Virtualizer's sc_module extension
// end_of_construction() is called after module construction
void sc_module::end_of_construction() {
    // Empty stub - end of construction callback not required for basic functionality
}

} // namespace sc_core

