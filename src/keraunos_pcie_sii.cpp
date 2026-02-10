#include "keraunos_pcie_sii.h"
#include <cstring>
#include <iostream>

namespace keraunos {
namespace pcie {

SiiBlock::SiiBlock(sc_core::sc_module_name name)
    : sc_module(name)
    , apb_adapter_("apb_adapter", apb_socket)
    , sii_memory_("sii_memory", 64 * 1024)  // 64KB
    , core_control_reg_("core_control", sii_memory_, CORE_CONTROL_OFFSET)
    , cfg_modified_reg_("cfg_modified", sii_memory_, CFG_MODIFIED_OFFSET)
    , bus_dev_num_reg_("bus_dev_num", sii_memory_, BUS_DEV_NUM_OFFSET)
    , cfg_modified_(0)
    , cii_modified_(0)
    , cii_clear_(0)
    , cfg_modified_sync_(0)
    , core_control_axi_(0)
    , bus_dev_num_axi_(0)
{
    // Bind SCML adapter to memory
    apb_adapter_(sii_memory_);
    
    // Configure register callbacks (AXI clock domain)
    // According to VZ_SCMLRef.md Section 2.5.4, use set_write_callback with SCML2_CALLBACK macro
    scml2::set_write_callback(core_control_reg_, SCML2_CALLBACK(core_control_write_callback), scml2::NEVER_SYNCING);
    scml2::set_write_callback(cfg_modified_reg_, SCML2_CALLBACK(cfg_modified_write_callback), scml2::NEVER_SYNCING);
    scml2::set_write_callback(bus_dev_num_reg_, SCML2_CALLBACK(bus_dev_num_write_callback), scml2::NEVER_SYNCING);
    
    // Register processes for CII tracking (PCIE core clock domain)
    SC_METHOD(cii_tracking_process);
    sensitive << cii_hv << cii_hdr_type << cii_hdr_addr;
    dont_initialize();
    
    SC_METHOD(cfg_modified_update_process);
    sensitive << pcie_core_clk.pos();
    async_reset_signal_is(reset_n, false);
    dont_initialize();
    
    // Clock domain crossing processes
    SC_METHOD(cdc_apb_to_pcie);
    sensitive << axi_clk.pos();
    dont_initialize();
    
    SC_METHOD(cdc_pcie_to_apb);
    sensitive << pcie_core_clk.pos();
    async_reset_signal_is(reset_n, false);
    dont_initialize();
    
    // Note: Output initialization moved to end_of_elaboration() 
    // to avoid writing to unbound ports during construction
}

SiiBlock::~SiiBlock() {
}

void SiiBlock::end_of_elaboration() {
    sc_module::end_of_elaboration();
    
    // Initialize outputs now that all ports are bound
    app_bus_num.write(0);
    app_dev_num.write(0);
    device_type.write(false);  // Default: EP mode
    sys_int.write(false);
    config_int.write(false);
}

/**
 * CII Tracking Process (Combinational)
 * 
 * This process tracks configuration space updates from the PCIe Controller
 * via the Configuration Intercept Interface (CII).
 * 
 * Operation:
 * - Monitors CII interface for config write transactions
 * - Only tracks first 128B of config space (address[11:7] == 0)
 * - Sets corresponding bit in cii_modified_ when register is written
 * - Type 0x04 (00100b) indicates configuration write transaction
 */
void SiiBlock::cii_tracking_process() {
    // Initialize cii_modified_ to all zeros
    cii_modified_ = 0;
    
    // CII tracking config space update (first 128B)
    // Check if CII header is valid and represents a config write
    if (cii_hv.read() && 
        (cii_hdr_type.read().to_uint() == 0x04) &&  // Type 00100b = config write
        ((cii_hdr_addr.read().to_uint() >> 7) == 0)) {  // Address[11:7] == 0 (first 128B)
        
        // Extract register index from address[6:2]
        // Each 32-bit register occupies 4 bytes, so address[6:2] gives register index
        uint8_t reg_index = (cii_hdr_addr.read().to_uint() >> 2) & 0x1F;  // Address[6:2]
        
        // Set the corresponding bit in cii_modified_ to indicate this register was modified
        sc_dt::sc_bv<32> modified_bits = cii_modified_;
        modified_bits[reg_index] = true;
        cii_modified_ = modified_bits;
    }
}

/**
 * Configuration Modified Update Process (Sequential, PCIE core clock domain)
 * 
 * This process updates the cfg_modified_ register based on:
 * 1. New modifications detected via CII (cii_modified_)
 * 2. Software clearing bits via RW1C write (cii_clear_)
 * 
 * The register follows RW1C (Read-Write-1-to-Clear) semantics:
 * - Reading returns current modified bits
 * - Writing 1 to a bit clears that bit
 * - Writing 0 has no effect
 * 
 * Formula: cfg_modified = (cfg_modified & ~cii_clear) | cii_modified
 */
void SiiBlock::cfg_modified_update_process() {
    if (!reset_n.read()) {
        // Reset: clear all modified bits
        cfg_modified_sync_ = 0;
        config_int.write(false);
    } else {
        // Update cfg_modified register:
        // - Clear bits where software wrote 1 (RW1C)
        // - Set bits where CII detected modification
        cfg_modified_sync_ = (cfg_modified_sync_ & ~cii_clear_) | cii_modified_;
        
        // Generate interrupt if any bit is set
        // This interrupt is sent to SMC PLIC for firmware handling
        bool interrupt_active = cfg_modified_sync_.or_reduce();
        config_int.write(interrupt_active);
    }
}

/**
 * Core Control Register Write Callback (AXI clock domain)
 * 
 * Handles writes to Core Control Register which contains:
 * - Device type (EP=0, RP=4)
 * - Other control bits
 * 
 * The value is synchronized to PCIe core clock domain via CDC
 */
bool SiiBlock::core_control_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    core_control_axi_ = data;
    
    // Extract device type and update output (will be synchronized via CDC)
    uint8_t dev_type = data & CORE_CONTROL_DEVICE_TYPE_MASK;
    // Note: device_type output will be updated in cdc_apb_to_pcie process
    (void)dev_type;  // Suppress unused variable warning
    return true;  // Return true to indicate successful access
}

/**
 * Configuration Modified Register Write Callback (AXI clock domain)
 * 
 * Handles RW1C (Read-Write-1-to-Clear) writes to cfg_modified register.
 * 
 * RW1C Semantics:
 * - Writing 1 to a bit clears that bit
 * - Writing 0 has no effect
 * - Reading returns current modified bits
 * 
 * The clear operation is synchronized to PCIe core clock domain
 */
bool SiiBlock::cfg_modified_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // RW1C: bits written as 1 will clear the corresponding cfg_modified bits
    // Extract bits that are being cleared (where data == 1)
    uint32_t clear_bits = data;
    
    // Synchronize clear bits to PCIe core clock domain
    // This will be handled in the CDC process
    cii_clear_ = clear_bits;
    
    // Note: The actual clearing happens in cfg_modified_update_process
    // which runs in PCIe core clock domain
    return true;  // Return true to indicate successful access
}

/**
 * Bus/Device Number Register Write Callback (AXI clock domain)
 * 
 * Handles writes to Bus/Device Number Register.
 * This register assigns bus and device numbers to the PCIe controller.
 * 
 * Format (typical):
 * - [7:0] = Device number
 * - [15:8] = Bus number
 */
bool SiiBlock::bus_dev_num_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    bus_dev_num_axi_ = data;
    
    // Extract bus and device numbers
    // Format may vary, but typically:
    // app_dev_num = data[7:0]
    // app_bus_num = data[15:8]
    // Note: These will be synchronized via CDC
    return true;  // Return true to indicate successful access
}

/**
 * Clock Domain Crossing: APB (AXI clock) -> PCIe Core Clock
 * 
 * Synchronizes configuration register writes from APB (AXI clock domain)
 * to PCIe core clock domain where they drive the PCIe Controller.
 * 
 * This implements the clock domain crossing logic mentioned in the spec:
 * "The clock domain crossing logic is inserted right before APB port
 *  attached to SII block."
 */
void SiiBlock::cdc_apb_to_pcie() {
    // Synchronize core control register
    core_control_pcie_.write(core_control_axi_);
    
    // Extract device type and drive output
    uint8_t dev_type = core_control_axi_ & CORE_CONTROL_DEVICE_TYPE_MASK;
    device_type.write(dev_type == CORE_CONTROL_DEVICE_TYPE_RP);
    
    // Synchronize bus/device number register
    bus_dev_num_pcie_.write(bus_dev_num_axi_);
    
    // Extract and drive bus/device numbers
    // Assuming format: [15:8] = bus, [7:0] = device
    app_bus_num.write((bus_dev_num_axi_ >> 8) & 0xFF);
    app_dev_num.write(bus_dev_num_axi_ & 0xFF);
}

/**
 * Clock Domain Crossing: PCIe Core Clock -> APB (AXI clock)
 * 
 * Synchronizes cfg_modified register from PCIe core clock domain
 * back to AXI clock domain for APB reads.
 * 
 * This allows firmware to read the current state of modified bits
 * via APB access.
 * 
 * Note: In a real implementation, this would use proper CDC synchronizers
 * (e.g., 2-stage synchronizer) to avoid metastability issues.
 */
void SiiBlock::cdc_pcie_to_apb() {
    if (!reset_n.read()) {
        cfg_modified_ = 0;
    } else {
        // Synchronize cfg_modified back to AXI domain for reads
        // Note: In a real implementation, this would use proper CDC synchronizers
        cfg_modified_ = cfg_modified_sync_;
        
        // Update the SCML register for APB reads
        // This allows firmware to read the current state via APB
        // Write directly to memory at the register offset
        uint32_t value = cfg_modified_sync_.to_uint();
        uint8_t* data_ptr = reinterpret_cast<uint8_t*>(&value);
        sii_memory_.write(CFG_MODIFIED_OFFSET, data_ptr, sizeof(uint32_t));
    }
}

} // namespace pcie
} // namespace keraunos
