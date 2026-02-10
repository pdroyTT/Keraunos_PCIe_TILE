#include "keraunos_pcie_sii.h"
#include <cstring>

namespace keraunos {
namespace pcie {

SiiBlock::SiiBlock()
    : cii_hv_(false), reset_n_(false)
    , config_int_(false), device_type_(false), sys_int_(false)
    , app_bus_num_(0), app_dev_num_(0)
    , cfg_modified_(0), cii_clear_(0)
    , sii_memory_("sii_memory", 65536)  // 64KB SCML2 memory
{
}

/**
 * update() -- CII tracking, cfg_modified update, interrupt generation.
 *
 * Called by the tile's SC_METHOD after all input setters have been
 * invoked and before output getters are read.  This single method
 * replaces the three SC_METHODs in the backup_original sc_module
 * implementation:
 *
 *   1.  cii_tracking_process      (combinational, sensitive to CII inputs)
 *   2.  cfg_modified_update_process (sequential,   posedge pcie_core_clk)
 *   3.  cdc_pcie_to_apb            (sequential,   posedge pcie_core_clk)
 *
 * Because the refactored SiiBlock is a plain C++ class driven by the
 * tile's delta-cycle-accurate SC_METHOD, one invocation of update()
 * is equivalent to one clock edge in both domains.
 */
void SiiBlock::update() {
    // ---- Phase 0: Reset ------------------------------------------------
    if (!reset_n_) {
        cfg_modified_ = 0;
        cii_clear_    = 0;
        config_int_   = false;
        return;
    }

    // ---- Phase 1: CII tracking (combinational) -------------------------
    // Equivalent to cii_tracking_process() in the backup_original.
    // Detects PCIe config-space writes reported via the Configuration
    // Intercept Interface (CII).  Only the first 128 bytes of config
    // space are tracked (address[11:7] == 0).  Type 0x04 (00100b) is
    // a configuration write transaction.
    uint32_t cii_new_bits = 0;

    if (cii_hv_ &&
        (cii_hdr_type_.to_uint() == 0x04) &&            // config write
        ((cii_hdr_addr_.to_uint() >> 7) == 0)) {        // first 128B
        // Register index = address[6:2] (each 32-bit register is 4 bytes)
        uint8_t reg_index = (cii_hdr_addr_.to_uint() >> 2) & 0x1F;
        cii_new_bits = (1u << reg_index);
    }

    // ---- Phase 2: cfg_modified update (sequential) ----------------------
    // Equivalent to cfg_modified_update_process().
    //   cfg_modified = (cfg_modified & ~cii_clear) | cii_new_bits
    //
    // RW1C semantics: bits written as 1 via APB are cleared;
    //                 new CII modifications are ORed in.
    cfg_modified_ = (cfg_modified_ & ~cii_clear_) | cii_new_bits;
    cii_clear_ = 0;  // clear pending after applying

    // ---- Phase 3: Interrupt generation ----------------------------------
    // config_int is asserted when any bit in cfg_modified is set.
    // This interrupt is routed to SMC PLIC for firmware handling.
    config_int_ = (cfg_modified_ != 0);

    // ---- Phase 4: CDC sync to SCML memory (for APB readback) ------------
    // Equivalent to cdc_pcie_to_apb().
    // Write the current cfg_modified value into the SCML memory at
    // CFG_MODIFIED_OFFSET so that APB reads return the live value.
    uint32_t mod_val = cfg_modified_;
    for (unsigned i = 0; i < sizeof(uint32_t); i++) {
        sii_memory_[CFG_MODIFIED_OFFSET + i] =
            static_cast<uint8_t>((mod_val >> (i * 8)) & 0xFF);
    }
}

/**
 * process_apb_access() -- handles TLM read/write from the SMN-IO switch.
 *
 * Register map (relative offsets within 64KB SII space):
 *   0x0000  CORE_CONTROL   -- [2:0] device_type (0=EP, 4=RP)
 *   0x0004  CFG_MODIFIED   -- RW1C: config-modified bitmask
 *   0x0008  BUS_DEV_NUM    -- [15:8] bus number, [7:0] device number
 *
 * Note: the SMN-IO switch currently passes the full SMN address to this
 * callback (address passthrough), so offsets >64KB will return
 * TLM_ADDRESS_ERROR_RESPONSE.  When the switch is fixed to strip the
 * base address, all registers will be accessible.
 */
void SiiBlock::process_apb_access(tlm::tlm_generic_payload& trans,
                                   sc_core::sc_time& delay) {
    uint32_t offset   = static_cast<uint32_t>(trans.get_address());
    uint32_t len      = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();

    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        if (offset + len <= sii_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                data_ptr[i] = sii_memory_[offset + i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        if (offset + len <= sii_memory_.get_size()) {
            // Store raw data into SCML memory
            for (uint32_t i = 0; i < len; i++) {
                sii_memory_[offset + i] = data_ptr[i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);

            // --- Register-specific side effects ---
            if (len >= 4) {
                uint32_t wdata;
                std::memcpy(&wdata, data_ptr, sizeof(uint32_t));

                if (offset == CORE_CONTROL_OFFSET) {
                    // Core Control: extract device type [2:0]
                    device_type_ =
                        ((wdata & CORE_CONTROL_DEVICE_TYPE_MASK)
                         == CORE_CONTROL_DEVICE_TYPE_RP);
                }
                else if (offset == CFG_MODIFIED_OFFSET) {
                    // RW1C: bits written as 1 schedule a clear of the
                    // corresponding cfg_modified bits.  The actual clear
                    // happens in update() (next delta cycle).
                    cii_clear_ |= wdata;
                }
                else if (offset == BUS_DEV_NUM_OFFSET) {
                    // Bus/Device number: [15:8]=bus, [7:0]=device
                    app_bus_num_ = (wdata >> 8) & 0xFF;
                    app_dev_num_ = wdata & 0xFF;
                }
            }
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

} // namespace pcie
} // namespace keraunos
