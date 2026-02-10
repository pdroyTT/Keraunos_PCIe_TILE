#ifndef KERAUNOS_PCIE_TILE_H
#define KERAUNOS_PCIE_TILE_H

#include "keraunos_pcie_common.h"
#include "keraunos_pcie_inbound_tlb.h"
#include "keraunos_pcie_outbound_tlb.h"
#include "keraunos_pcie_msi_relay.h"
#include "keraunos_pcie_noc_pcie_switch.h"
#include "keraunos_pcie_noc_io_switch.h"
#include "keraunos_pcie_smn_io_switch.h"
#include "keraunos_pcie_sii.h"
#include "keraunos_pcie_config_reg.h"
#include "keraunos_pcie_clock_reset.h"
#include "keraunos_pcie_pll_cgm.h"
#include "keraunos_pcie_phy.h"
#include "keraunos_pcie_external_interfaces.h"
#include <scml2.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <sc_dt.h>
#include <vector>

namespace keraunos {
namespace pcie {

/**
 * Keraunos PCI Express Tile
 * 
 * Top-level module that instantiates and connects all PCIE Tile components
 */
class KeraunosPcieTile : public sc_core::sc_module {
public:
    // External AXI interfaces
    // Note: Using 64-bit sockets for SCML2 compatibility, but addresses are masked to 52 bits
    tlm::tlm_base_target_socket<64> noc_n_target;              // NOC-N target (256-bit, 52-bit addr masked to 64-bit)
    tlm_utils::simple_target_socket<KeraunosPcieTile, 64> noc_n_initiator;       // NOC-N initiator port (receives FROM internal switches, sends TO external)
    tlm::tlm_base_target_socket<64> smn_n_target;              // SMN-N target (64-bit, 52-bit addr masked to 64-bit)
    tlm_utils::simple_target_socket<KeraunosPcieTile, 64> smn_n_initiator;       // SMN-N initiator port (receives FROM internal switches, sends TO external)
    
    // PCIe Controller interface
    tlm::tlm_base_target_socket<64> pcie_controller_target;   // To PCIe Controller (receives inbound transactions)
    tlm_utils::simple_target_socket<KeraunosPcieTile, 64> pcie_controller_initiator;   // From PCIe Controller (receives FROM internal, sends TO controller)
    
    // PCIe Controller CII (Configuration Intercept Interface) signals
    sc_core::sc_in<bool> pcie_cii_hv;                          // CII Header Valid (from PCIe Controller)
    sc_core::sc_in<sc_dt::sc_bv<5> > pcie_cii_hdr_type;       // CII Header Type [4:0] (from PCIe Controller)
    sc_core::sc_in<sc_dt::sc_bv<12> > pcie_cii_hdr_addr;      // CII Header Address [11:0] (from PCIe Controller)
    
    // PCIe Controller clock/reset (from PCIe Controller)
    sc_core::sc_in<bool> pcie_core_clk;                       // PCIe core clock from controller
    sc_core::sc_in<bool> pcie_controller_reset_n;             // Reset from controller
    
    // PCIe Controller interrupt inputs (from PCIe Controller)
    sc_core::sc_in<bool> pcie_flr_request;                    // FLR request from controller
    sc_core::sc_in<bool> pcie_hot_reset;                      // Hot reset from controller
    sc_core::sc_in<bool> pcie_ras_error;                      // RAS error from controller
    sc_core::sc_in<bool> pcie_dma_completion;                 // DMA completion from controller
    sc_core::sc_in<bool> pcie_misc_int;                       // Misc interrupt from controller
    
    // SII output signals (to PCIe Controller)
    sc_core::sc_out<uint8_t> pcie_app_bus_num;                // Application bus number (to PCIe Controller)
    sc_core::sc_out<uint8_t> pcie_app_dev_num;                // Application device number (to PCIe Controller)
    sc_core::sc_out<bool> pcie_device_type;                   // Device type (0=EP, 4=RP) (to PCIe Controller)
    sc_core::sc_out<bool> pcie_sys_int;                       // Legacy interrupt control (to PCIe Controller)
    
    // AXI clock (for APB access to SII block)
    sc_core::sc_in<bool> axi_clk;                             // AXI clock
    
    // External control signals
    sc_core::sc_in<bool> cold_reset_n;                        // Cold reset
    sc_core::sc_in<bool> warm_reset_n;                        // Warm reset
    sc_core::sc_in<bool> isolate_req;                          // Isolation request
    
    // Interrupt outputs
    sc_core::sc_out<bool> function_level_reset;         // FLR request
    sc_core::sc_out<bool> hot_reset_requested;         // Hot reset request
    sc_core::sc_out<bool> config_update;                // Config update interrupt
    sc_core::sc_out<bool> ras_error;                    // RAS error
    sc_core::sc_out<bool> dma_completion;               // DMA completion
    sc_core::sc_out<bool> controller_misc_int;          // Controller misc interrupt
    sc_core::sc_out<sc_dt::sc_bv<3> > noc_timeout;      // NOC timeout (3-bit)
    
    SC_HAS_PROCESS(KeraunosPcieTile);
    
    KeraunosPcieTile(sc_core::sc_module_name name);
    virtual ~KeraunosPcieTile();
    
protected:
    // Forward transport methods for pass-through target sockets
    // These sockets receive from internal initiators and need to forward to external/test
    void noc_n_initiator_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int noc_n_initiator_transport_dbg(tlm::tlm_generic_payload& trans);
    void smn_n_initiator_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int smn_n_initiator_transport_dbg(tlm::tlm_generic_payload& trans);
    void pcie_controller_initiator_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int pcie_controller_initiator_transport_dbg(tlm::tlm_generic_payload& trans);
    
protected:
    // Component instances
    // Switches
    NocPcieSwitch* noc_pcie_switch_;
    NocIoSwitch* noc_io_switch_;
    SmnIoSwitch* smn_io_switch_;
    
    // TLBs
    TLBSysIn0* tlb_sys_in0_;
    TLBAppIn0* tlb_app_in0_[4];  // 4 instances
    TLBAppIn1* tlb_app_in1_;
    TLBSysOut0* tlb_sys_out0_;
    TLBAppOut0* tlb_app_out0_;
    TLBAppOut1* tlb_app_out1_;
    
    // MSI Relay
    MsiRelayUnit* msi_relay_;
    
    // Configuration and Control
    SiiBlock* sii_block_;
    ConfigRegBlock* config_reg_;
    ClockResetControl* clock_reset_ctrl_;
    PllCgm* pll_cgm_;
    PciePhy* pcie_phy_;
    
    // External Interfaces
    // Note: External interface wrappers removed - switches connect directly to top-level ports
    // NocNInterface* noc_n_interface_;  // No longer used
    // SmnNInterface* smn_n_interface_;  // No longer used
    
    // Internal signals
    sc_core::sc_signal<bool> system_ready_;
    sc_core::sc_signal<bool> pcie_outbound_app_enable_;
    sc_core::sc_signal<bool> pcie_inbound_app_enable_;
    sc_core::sc_signal<bool> msix_enable_;
    sc_core::sc_signal<bool> msix_mask_;
    sc_core::sc_signal<sc_dt::sc_bv<16> > setip_;
    
    // Clock signals
    sc_core::sc_signal<bool> pcie_clock_;
    sc_core::sc_signal<bool> ref_clock_;
    
    // Reset signals
    sc_core::sc_signal<bool> pcie_sii_reset_ctrl_;
    sc_core::sc_signal<bool> pcie_reset_ctrl_;
    
    // Internal timeout signals (for combining into noc_timeout)
    sc_core::sc_signal<bool> noc_timeout_read_;
    sc_core::sc_signal<bool> noc_timeout_write_;
    sc_core::sc_signal<bool> smn_timeout_;
    
    // Intermediate signals for PCIe Controller interrupt passthrough
    sc_core::sc_signal<bool> flr_sig_;
    sc_core::sc_signal<bool> hot_reset_sig_;
    sc_core::sc_signal<bool> ras_error_sig_;
    sc_core::sc_signal<bool> dma_completion_sig_;
    sc_core::sc_signal<bool> misc_int_sig_;
    
    // Intermediate signals for SII output passthrough
    sc_core::sc_signal<uint8_t> app_bus_num_sig_;
    sc_core::sc_signal<uint8_t> app_dev_num_sig_;
    sc_core::sc_signal<bool> device_type_sig_;
    sc_core::sc_signal<bool> sys_int_sig_;
    sc_core::sc_signal<bool> config_int_sig_;
    
    // Connection helper methods
    void connect_components();
    
    // Process to combine timeout signals
    void combine_timeout_process();
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_TILE_H

