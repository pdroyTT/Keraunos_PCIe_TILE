// Stub implementation for Virtualizer SystemC event extension
// This is needed because Virtualizer's SystemC headers reference m_ongoing_notify_time
// that isn't in standard SystemC libraries

#include <sysc/kernel/sc_event.h>

namespace sc_core {

// Provide stub implementation for Virtualizer's sc_event extension
// m_ongoing_notify_time is a static const sc_time* member used by Virtualizer extensions
// Initialize it to nullptr (immediate notification)
const sc_time* sc_event::m_ongoing_notify_time = nullptr;

} // namespace sc_core
