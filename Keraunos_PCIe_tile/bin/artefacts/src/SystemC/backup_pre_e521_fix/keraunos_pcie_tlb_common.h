#ifndef KERAUNOS_PCIE_TLB_COMMON_H
#define KERAUNOS_PCIE_TLB_COMMON_H

#include <systemc>
#include <tlm>
#include <cstdint>
#include <sc_dt.h>

namespace keraunos {
namespace pcie {

// TLB Entry Structure (64 bytes per entry)
// Based on Table 14: [0] Valid, [63:12] ADDR, [511:256] ATTR
struct TlbEntry {
    bool valid;                    // [0] Valid bit
    uint64_t addr;                 // [63:12] Address (52 bits)
    sc_dt::sc_bv<256> attr;       // [255:0] Attribute for AxUSER field
    
    TlbEntry() : valid(false), addr(0), attr(0) {}
};

// Invalid address constant to return DECERR
const uint64_t INVALID_ADDRESS_DECERR = 0xFFFFFFFFFFFFFFFFULL;

// TLB Types
enum class TlbType {
    TLBSysOut0,    // Outbound System TLB (16 entries, 64KB page)
    TLBAppOut0,    // Outbound Application TLB (16 entries, 16TB page)
    TLBAppOut1,    // Outbound Application TLB (16 entries, 64KB page)
    TLBSysIn0,     // Inbound System TLB (64 entries, 16KB page)
    TLBAppIn0,     // Inbound Application TLB (64 entries, 16MB page)
    TLBAppIn1      // Inbound Application TLB (64 entries, 8GB page)
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_TLB_COMMON_H

