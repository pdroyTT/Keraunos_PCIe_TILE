#ifndef KERAUNOS_PCIE_SII_H
#define KERAUNOS_PCIE_SII_H

#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <scml2/reg.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * System Information Interface (SII) Block
 * 
 * Provides configuration information to PCI Express controller and tracks
 * configuration updates via Configuration Intercept Interface (CII).
 * 
 * Key Features:
 * - 64KB configuration register space accessible via APB
 * - CII tracking for config space updates (first 128B)
 * - Configuration update interrupt generation (to SMC PLIC)
 * - Bus/device number assignment for controller
 * - Clock domain crossing (PCIE core clock <-> AXI clock)
 * 
 * Operation:
 * 1. SMC firmware writes configuration registers via APB (AXI clock domain)
 * 2. Clock domain crossing logic transfers writes to PCIe core clock domain
 * 3. SII registers drive configuration signals to PCIe Controller IP
 * 4. When PCIe Controller receives config writes from host, CII interface
 *    reports the update
 * 5. SII tracks which registers were modified and generates interrupt
 * 6. Interrupt is sent to SMC PLIC for firmware handling
 */
class SiiBlock : public sc_core::sc_module {
public:
    // APB target socket for configuration access (AXI clock domain)
    tlm::tlm_base_target_socket<32> apb_socket;
    
    // CII interface signals (from PCIe Controller, PCIe core clock domain)
    sc_core::sc_in<bool> cii_hv;                        // CII Header Valid
    sc_core::sc_in<sc_dt::sc_bv<5> > cii_hdr_type;     // CII Header Type [4:0]
    sc_core::sc_in<sc_dt::sc_bv<12> > cii_hdr_addr;    // CII Header Address [11:0]
    
    // Clock inputs
    sc_core::sc_in<bool> pcie_core_clk;                // PCIe core clock (from controller)
    sc_core::sc_in<bool> axi_clk;                      // AXI clock (for APB access)
    
    // Reset inputs
    sc_core::sc_in<bool> reset_n;                      // Reset (active low)
    
    // Interrupt output (to SMC PLIC via top-level tile)
    sc_core::sc_out<bool> config_int;                   // Configuration update interrupt
    
    // SII output signals to PCIe Controller (PCIE core clock domain)
    sc_core::sc_out<uint8_t> app_bus_num;               // Application bus number
    sc_core::sc_out<uint8_t> app_dev_num;               // Application device number
    sc_core::sc_out<bool> device_type;                  // Device type (0=EP, 4=RP)
    sc_core::sc_out<bool> sys_int;                      // Legacy interrupt control
    
    SC_HAS_PROCESS(SiiBlock);
    
    SiiBlock(sc_core::sc_module_name name);
    virtual ~SiiBlock();
    
    // Initialize outputs after elaboration (when ports are bound)
    virtual void end_of_elaboration();
    
protected:
    // SCML port adapter (AXI clock domain)
    scml2::tlm2_gp_target_adapter<32> apb_adapter_;
    
    // SCML memory for SII register space (64KB, AXI clock domain)
    scml2::memory<uint8_t> sii_memory_;
    
    // SII Configuration Registers (mapped in sii_memory_)
    // Base address: 0x18100000 + 0x04000 (APB demux offset)
    scml2::reg<uint32_t> core_control_reg_;            // Core Control Register
    scml2::reg<uint32_t> cfg_modified_reg_;            // Config Modified Register (RW1C)
    scml2::reg<uint32_t> bus_dev_num_reg_;             // Bus/Device Number Register
    
    // Register field accessors
    static const uint32_t CORE_CONTROL_OFFSET = 0x0000;
    static const uint32_t CFG_MODIFIED_OFFSET = 0x0004;  // RW1C register
    static const uint32_t BUS_DEV_NUM_OFFSET = 0x0008;
    
    // Core Control Register fields
    static const uint32_t CORE_CONTROL_DEVICE_TYPE_MASK = 0x7;  // [2:0]
    static const uint32_t CORE_CONTROL_DEVICE_TYPE_EP = 0x0;
    static const uint32_t CORE_CONTROL_DEVICE_TYPE_RP = 0x4;
    
    // CII tracking processes
    void cii_tracking_process();                        // Combinational CII tracking
    void cfg_modified_update_process();                // Sequential cfg_modified update
    
    // Register callbacks
    // According to VZ_SCMLRef.md Section 2.5.4, write callbacks use:
    // bool writeCallback(const DT& data, const DT& byteEnables, sc_core::sc_time&, int tag)
    bool core_control_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool cfg_modified_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool bus_dev_num_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    
    // Clock domain crossing helpers
    void cdc_apb_to_pcie();                            // APB -> PCIe clock domain
    void cdc_pcie_to_apb();                            // PCIe -> APB clock domain
    
    // Internal state (PCIE core clock domain)
    sc_dt::sc_bv<32> cfg_modified_;                    // Configuration modified bits
    sc_dt::sc_bv<32> cii_modified_;                    // CII modified bits (combinational)
    sc_dt::sc_bv<32> cii_clear_;                       // Software clear bits (from APB write)
    
    // Synchronized registers (PCIE core clock domain)
    sc_dt::sc_bv<32> cfg_modified_sync_;               // Synchronized cfg_modified
    
    // Internal state (AXI clock domain)
    uint32_t core_control_axi_;                        // Core control (AXI domain)
    uint32_t bus_dev_num_axi_;                         // Bus/dev num (AXI domain)
    
    // Clock domain crossing signals
    sc_core::sc_signal<uint32_t> core_control_pcie_;  // Core control (PCIE domain)
    sc_core::sc_signal<uint32_t> bus_dev_num_pcie_;   // Bus/dev num (PCIE domain)
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_SII_H

