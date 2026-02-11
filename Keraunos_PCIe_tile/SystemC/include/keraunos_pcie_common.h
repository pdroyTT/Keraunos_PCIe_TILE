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

// Clock frequencies (in Hz) - constexpr for compile-time evaluation
constexpr uint64_t PCIE_CLOCK_FREQ = 1000000000ULL;      // 1.0 GHz
constexpr uint64_t NOC_CLOCK_FREQ = 1650000000ULL;       // 1.65 GHz
constexpr uint64_t SOC_CLOCK_FREQ = 400000000ULL;        // 400 MHz
constexpr uint64_t AHB_CLOCK_FREQ = 500000000ULL;        // 500 MHz
constexpr uint64_t REF_CLOCK_FREQ = 100000000ULL;        // 100 MHz

// Reset types
enum class ResetType {
    COLD_RESET,      // Management Reset + Main Reset
    WARM_RESET,      // Main Reset only
    FUNCTION_LEVEL,  // FLR
    HOT_RESET        // Hot Reset / Link-down Reset
};

// Address map constants - constexpr for compile-time evaluation and better optimization
// Original SMN address map per design spec (Appendix B.5)
constexpr uint64_t SMN_BASE = 0x18000000ULL;
constexpr uint64_t MSI_RELAY_BASE = 0x18000000ULL;       // 256KB: 0x18000000-0x1803FFFF (MSI Relay Config, 8 PF × 16KB)
constexpr uint64_t CONFIG_REG_BASE = 0x18040000ULL;      // 64KB: 0x18040000-0x1804FFFF (Config Reg Block: TLBs + status)
constexpr uint64_t TLB_CONFIG_BASE = 0x18040000ULL;      // Same as CONFIG_REG_BASE
constexpr uint64_t SMN_IO_CSR_BASE = 0x18050000ULL;      // 64KB: 0x18050000-0x1805FFFF (SMN-IO Fabric CSR)
constexpr uint64_t SERDES_AHB_BASE = 0x18080000ULL;      // 256KB: 0x18080000-0x180BFFFF (SerDes AHB0)
constexpr uint64_t SERDES_APB_BASE = 0x180C0000ULL;      // 256KB: 0x180C0000-0x180FFFFF (SerDes APB0)
constexpr uint64_t SII_BASE = 0x18100000ULL;             // 1MB: 0x18100000-0x181FFFFF (SII Config, APB Demux)
// NOC-IO switch address space (Appendix B.6)
constexpr uint64_t MSI_RELAY_MSI_BASE = 0x18800000ULL;   // 1MB: 0x18800000-0x188FFFFF (MSI Relay MSI generation)
constexpr uint64_t TLB_APP_OUTBOUND_BASE = 0x18900000ULL;// 1MB: 0x18900000-0x189FFFFF (TLB App Outbound DBI)
// TLB Sys0 Outbound data path (via SMN-IO)
constexpr uint64_t TLB_SYS_OUTBOUND_BASE = 0x18400000ULL;// 1MB: 0x18400000-0x184FFFFF

// System Ready register address (special routing)
constexpr uint64_t SYSTEM_READY_ADDR = 0xE000000000000000ULL;  // AxADDR[63:60] = 0xE, [59:7] = 0

// Outstanding request tracking
struct OutstandingRequest {
    uint64_t id;
    uint64_t addr;
    bool is_read;
    sc_core::sc_time timestamp;
    
    OutstandingRequest() : id(0), addr(0), is_read(false), timestamp(sc_core::SC_ZERO_TIME) {}
};

// AxUSER TLM extension — carries sideband (AxUSER) info through TLM transactions.
// Used by outbound TLBs to pass TLB ATTR to NOC-PCIE for BME qualification.
struct AxUserExtension : public tlm::tlm_extension<AxUserExtension> {
    sc_dt::sc_bv<256> axuser;

    AxUserExtension() : axuser(0) {}

    tlm::tlm_extension_base* clone() const override {
        auto* e = new AxUserExtension;
        e->axuser = axuser;
        return e;
    }
    void copy_from(const tlm::tlm_extension_base& other) override {
        axuser = static_cast<const AxUserExtension&>(other).axuser;
    }
};

// Address masking helpers for 52-bit addresses
// constexpr and noexcept for optimization
constexpr uint64_t ADDR_52BIT_MASK = 0x000FFFFFFFFFFFFFULL;  // 52-bit mask

[[nodiscard]] inline constexpr uint64_t mask_52bit_address(const uint64_t addr) noexcept {
    return addr & ADDR_52BIT_MASK;
}

inline void set_52bit_address(tlm::tlm_generic_payload& trans, const uint64_t addr) noexcept {
    trans.set_address(mask_52bit_address(addr));
}

[[nodiscard]] inline uint64_t get_52bit_address(const tlm::tlm_generic_payload& trans) noexcept {
    return mask_52bit_address(trans.get_address());
}

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_COMMON_H

