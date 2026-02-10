#ifndef KERAUNOS_PCIE_COMMON_H
#define KERAUNOS_PCIE_COMMON_H

#include <systemc>
#include <tlm>
#include <cstdint>
#include <sc_dt.h>

namespace keraunos {
namespace pcie {

// Address routing constants for NOC-PCIE switch
// Routing based on AxADDR[63:60]
enum class NocPcieRoute {
    TLB_APP_0 = 0x0,      // TLB App0/App1 (BAR0/1)
    TLB_APP_1 = 0x1,      // TLB App0/App1 (BAR4/5)
    DECERR_2 = 0x2,
    DECERR_3 = 0x3,
    TLB_SYS = 0x4,        // TLB Sys0
    DECERR_5 = 0x5,
    DECERR_6 = 0x6,
    DECERR_7 = 0x7,
    BYPASS_APP = 0x8,     // Bypass TLB (App)
    BYPASS_SYS = 0x9,     // Bypass TLB (Sys)
    DECERR_10 = 0xA,
    DECERR_11 = 0xB,
    DECERR_12 = 0xC,
    DECERR_13 = 0xD,
    STATUS_REG = 0xE,     // Status Register (special case)
    STATUS_REG_ALT = 0xF  // Status Register (alternative)
};

// Clock frequencies (in Hz)
const uint64_t PCIE_CLOCK_FREQ = 1000000000ULL;      // 1.0 GHz
const uint64_t NOC_CLOCK_FREQ = 1650000000ULL;       // 1.65 GHz
const uint64_t SOC_CLOCK_FREQ = 400000000ULL;        // 400 MHz
const uint64_t AHB_CLOCK_FREQ = 500000000ULL;        // 500 MHz
const uint64_t REF_CLOCK_FREQ = 100000000ULL;        // 100 MHz

// Reset types
enum class ResetType {
    COLD_RESET,      // Management Reset + Main Reset
    WARM_RESET,      // Main Reset only
    FUNCTION_LEVEL,  // FLR
    HOT_RESET        // Hot Reset / Link-down Reset
};

// Address map constants
const uint64_t MSI_RELAY_CSR_BASE = 0x18000000ULL;
const uint64_t TLB_CONFIG_BASE = 0x18040000ULL;
const uint64_t SII_BASE = 0x18100000ULL;
const uint64_t SMN_IO_CSR_BASE = 0x18050000ULL;
const uint64_t SERDES_APB_BASE = 0x180C0000ULL;
const uint64_t SERDES_AHB_BASE = 0x18080000ULL;
const uint64_t MSI_RELAY_MSI_BASE = 0x18800000ULL;
const uint64_t TLB_APP_OUTBOUND_BASE = 0x18900000ULL;

// System Ready register address (special routing)
const uint64_t SYSTEM_READY_ADDR = 0xE000000000000000ULL;  // AxADDR[63:60] = 0xE, [59:7] = 0

// Outstanding request tracking
struct OutstandingRequest {
    uint64_t id;
    uint64_t addr;
    bool is_read;
    sc_core::sc_time timestamp;
    
    OutstandingRequest() : id(0), addr(0), is_read(false), timestamp(sc_core::SC_ZERO_TIME) {}
};

// Address masking helpers for 52-bit addresses
// NOC-N and SMN-N use 52-bit addresses, but we use 64-bit sockets for SCML2 compatibility
// These functions mask bits [63:52] to ensure only 52-bit addresses are used
inline uint64_t mask_52bit_address(uint64_t addr) {
    // Mask upper 12 bits [63:52] to 0, keep lower 52 bits [51:0]
    return addr & 0x000FFFFFFFFFFFFFULL;  // 0x000FFFFFFFFFFFFF = 52 bits
}

inline void set_52bit_address(tlm::tlm_generic_payload& trans, uint64_t addr) {
    // Set address with masking to ensure only 52 bits are used
    trans.set_address(mask_52bit_address(addr));
}

inline uint64_t get_52bit_address(const tlm::tlm_generic_payload& trans) {
    // Get address and mask to ensure only 52 bits are used
    return mask_52bit_address(trans.get_address());
}

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_COMMON_H

