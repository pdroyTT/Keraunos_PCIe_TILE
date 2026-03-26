// Stub implementation for Virtualizer SystemC writer policy check
// This is needed because Virtualizer's SystemC headers reference writer policy symbols
// that aren't in standard SystemC libraries

#include <sysc/communication/sc_writer_policy.h>

namespace sc_core {

// Provide stub implementation for writer policy check static member
sc_writer_policy snps_sc_writer_policy_check::m_policy = SC_UNCHECKED_WRITERS;

// Provide stub implementation for writer policy check method
// Signature: bool do_check_write(sc_object* target_, bool value_changed_)
bool snps_sc_writer_policy_check::do_check_write(sc_object* target_, bool value_changed_) {
    // Empty stub - writer policy checks not required for basic functionality
    // Return true to allow writes
    (void)target_;
    (void)value_changed_;
    return true;
}

} // namespace sc_core
