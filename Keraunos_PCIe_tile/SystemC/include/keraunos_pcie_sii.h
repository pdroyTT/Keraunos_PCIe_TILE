#ifndef KERAUNOS_PCIE_SII_H
#define KERAUNOS_PCIE_SII_H

// REFACTORED: C++ class with SCML2 memory and CII tracking logic.
// CII tracking detects PCIe config space updates from the host, maintains
// a cfg_modified bitmask, and generates a config_update interrupt.

#include <scml2.h>
#include <scml2/memory.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <cstdint>

namespace keraunos {
namespace pcie {

class SiiBlock {
public:
    SiiBlock();
    ~SiiBlock() = default;

    // TLM access from SMN-IO switch (APB register interface)
    void process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);

    // --- Input setters (called by tile SC_METHOD on signal change) ---
    void set_cii_hv(bool val) { cii_hv_ = val; }
    void set_cii_hdr_type(sc_dt::sc_bv<5> val) { cii_hdr_type_ = val; }
    void set_cii_hdr_addr(sc_dt::sc_bv<12> val) { cii_hdr_addr_ = val; }
    void set_reset_n(bool val) { reset_n_ = val; }

    // Process all inputs and update outputs.
    // Must be called by the tile after all setters, before reading getters.
    // Implements:  CII tracking -> cfg_modified update -> interrupt generation
    //              -> SCML memory sync (CDC equivalent)
    void update();

    // --- Output getters (called by tile to drive output ports) ---
    bool get_config_int() const { return config_int_; }
    uint8_t get_app_bus_num() const { return app_bus_num_; }
    uint8_t get_app_dev_num() const { return app_dev_num_; }
    bool get_device_type() const { return device_type_; }
    bool get_sys_int() const { return sys_int_; }

private:
    // --- CII input state ---
    bool cii_hv_;
    bool reset_n_;
    sc_dt::sc_bv<5> cii_hdr_type_;
    sc_dt::sc_bv<12> cii_hdr_addr_;

    // --- Output state ---
    bool config_int_;                   // config_update interrupt (to SMC PLIC)
    bool device_type_;                  // EP=false, RP=true
    bool sys_int_;                      // Legacy interrupt control
    uint8_t app_bus_num_;
    uint8_t app_dev_num_;

    // --- CII tracking / interrupt state ---
    uint32_t cfg_modified_;             // Accumulated config-modified bitmask
    uint32_t cii_clear_;               // Pending RW1C clear bits from APB write

    // --- SCML2 memory for APB register space (64KB) ---
    scml2::memory<uint8_t> sii_memory_;

    // Register offsets within SII APB space
    static const uint32_t CORE_CONTROL_OFFSET = 0x0000;
    static const uint32_t CFG_MODIFIED_OFFSET = 0x0004;
    static const uint32_t BUS_DEV_NUM_OFFSET  = 0x0008;

    // Core Control register fields
    static const uint32_t CORE_CONTROL_DEVICE_TYPE_MASK = 0x7;  // [2:0]
    static const uint32_t CORE_CONTROL_DEVICE_TYPE_RP   = 0x4;
};

} // namespace pcie
} // namespace keraunos

#endif
