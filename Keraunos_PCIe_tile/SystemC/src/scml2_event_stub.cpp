// Stub implementation for Virtualizer SystemC event extension
// This is needed because Virtualizer's SystemC headers reference m_ongoing_notify_time
// that may not be present in all SystemC library versions.
//
// ROOT CAUSE OF E521 BUG:
//   The VDK runtime (libSnpsVP.so) also defines m_ongoing_notify_time and actively
//   manages it — setting it to the current notification time while inside an SCML2
//   callback to signal a "safe context" for immediate sc_event::notify() calls.
//   The DWC PCIe model (libpcie_2_0_SC_2_0-O.so) references this symbol as 'U'
//   (undefined, resolved at dynamic link time).
//
//   If this PCIE_TILE library is loaded before libSnpsVP.so (or its copy wins the
//   dynamic linker race), the DWC PCIe DLL resolves m_ongoing_notify_time to THIS
//   copy (always nullptr), not to libSnpsVP.so's copy (which SCML2 sets at callback
//   entry time). The E521 check therefore always fires spuriously from within valid
//   SCML2 callbacks (e.g. ResetConfigurationSpace -> InitRegs_DeviceType).
//
// FIX:
//   Declare this as __attribute__((weak)) so the dynamic linker always prefers
//   libSnpsVP.so's strong definition.  All DLLs (DWC PCIe, PCIE_TILE, etc.) will
//   then resolve to the single copy that SCML2 actually manages, restoring correct
//   E521 suppression inside SCML2 callback contexts.

#include <sysc/kernel/sc_event.h>

namespace sc_core {

__attribute__((weak)) const sc_time* sc_event::m_ongoing_notify_time = nullptr;

} // namespace sc_core
