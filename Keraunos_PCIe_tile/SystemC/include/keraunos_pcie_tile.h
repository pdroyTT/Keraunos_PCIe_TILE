#ifndef KERAUNOS_PCIE_TILE_H
#define KERAUNOS_PCIE_TILE_H

// REFACTORED: Top-level sc_module with ONLY external sockets
// All internal components are C++ classes
// Modern C++: Using smart pointers for automatic memory management

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
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <sc_dt.h>
#include <memory>
#include <array>

namespace keraunos {
namespace pcie {

/**
 * Keraunos PCI Express Tile (REFACTORED)
 * - ONLY external-facing TLM sockets (for test harness)
 * - All internal components are C++ classes (no internal sockets)
 * - Internal communication via function callbacks
 */
class KeraunosPcieTile : public sc_core::sc_module {
public:
    // ========================================================================
    // EXTERNAL SOCKETS
    // Target sockets: receive transactions from external (testbench/fabric)
    // Initiator sockets: forward transactions to external (testbench/fabric)
    // ========================================================================
    // Target sockets (inbound from external)
    tlm_utils::simple_target_socket<KeraunosPcieTile, 64> noc_n_target;
    tlm_utils::simple_target_socket<KeraunosPcieTile, 64> smn_n_target;
    tlm_utils::simple_target_socket<KeraunosPcieTile, 64> pcie_controller_target;
    // Initiator sockets (outbound to external)
    tlm_utils::simple_initiator_socket<KeraunosPcieTile, 64> noc_n_initiator;
    tlm_utils::simple_initiator_socket<KeraunosPcieTile, 64> smn_n_initiator;
    tlm_utils::simple_initiator_socket<KeraunosPcieTile, 64> pcie_controller_initiator;
    
    // Control signal ports
    sc_core::sc_in<bool> cold_reset_n;
    sc_core::sc_in<bool> warm_reset_n;
    sc_core::sc_in<bool> isolate_req;
    sc_core::sc_in<bool> pcie_cii_hv;
    sc_core::sc_in<sc_dt::sc_bv<5>> pcie_cii_hdr_type;
    sc_core::sc_in<sc_dt::sc_bv<12>> pcie_cii_hdr_addr;
    sc_core::sc_in<bool> pcie_core_clk;
    sc_core::sc_in<bool> pcie_controller_reset_n;
    sc_core::sc_in<bool> pcie_flr_request;
    sc_core::sc_in<bool> pcie_hot_reset;
    sc_core::sc_in<bool> pcie_ras_error;
    sc_core::sc_in<bool> pcie_dma_completion;
    sc_core::sc_in<bool> pcie_misc_int;
    
    // Output signals
    sc_core::sc_out<uint8_t> pcie_app_bus_num;
    sc_core::sc_out<uint8_t> pcie_app_dev_num;
    sc_core::sc_out<bool> pcie_device_type;
    sc_core::sc_out<bool> pcie_sys_int;
    sc_core::sc_in<bool> axi_clk;
    sc_core::sc_out<bool> function_level_reset;
    sc_core::sc_out<bool> hot_reset_requested;
    sc_core::sc_out<bool> config_update;
    sc_core::sc_out<bool> ras_error;
    sc_core::sc_out<bool> dma_completion;
    sc_core::sc_out<bool> controller_misc_int;
    sc_core::sc_out<sc_dt::sc_bv<3>> noc_timeout;
    
    SC_HAS_PROCESS(KeraunosPcieTile);
    
    KeraunosPcieTile(sc_core::sc_module_name name);
    ~KeraunosPcieTile() override;  // override keyword for clarity
    
    void end_of_elaboration() override;  // override keyword
    
protected:
    // Top-level socket transport methods (target sockets only - initiator sockets forward outward)
    void noc_n_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void smn_n_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void pcie_controller_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Signal update process
    void signal_update_process();
    
    // Helper method to update modules that depend on config registers
    void update_config_dependent_modules();
    
protected:
    // ========================================================================
    // INTERNAL COMPONENTS (C++ classes - NO sockets!)
    // Using std::unique_ptr for automatic memory management (RAII)
    // ========================================================================
    std::unique_ptr<NocPcieSwitch> noc_pcie_switch_;
    std::unique_ptr<NocIoSwitch> noc_io_switch_;
    std::unique_ptr<SmnIoSwitch> smn_io_switch_;
    std::unique_ptr<TLBSysIn0> tlb_sys_in0_;
    std::array<std::unique_ptr<TLBAppIn0>, 4> tlb_app_in0_;  // Use std::array for bounds safety
    std::unique_ptr<TLBAppIn1> tlb_app_in1_;
    std::unique_ptr<TLBSysOut0> tlb_sys_out0_;
    std::unique_ptr<TLBAppOut0> tlb_app_out0_;
    std::unique_ptr<TLBAppOut1> tlb_app_out1_;
    std::unique_ptr<MsiRelayUnit> msi_relay_;
    std::unique_ptr<SiiBlock> sii_block_;
    std::unique_ptr<ConfigRegBlock> config_reg_;
    std::unique_ptr<ClockResetControl> clock_reset_ctrl_;
    std::unique_ptr<PllCgm> pll_cgm_;
    std::unique_ptr<PciePhy> pcie_phy_;
    
    // Internal signals
    sc_core::sc_signal<bool> system_ready_;
    sc_core::sc_signal<bool> pcie_outbound_app_enable_;
    sc_core::sc_signal<bool> pcie_inbound_app_enable_;
    sc_core::sc_signal<bool> msix_enable_;
    sc_core::sc_signal<bool> msix_mask_;
    sc_core::sc_signal<sc_dt::sc_bv<16>> setip_;
    sc_core::sc_signal<bool> pcie_clock_;
    sc_core::sc_signal<bool> ref_clock_;
    sc_core::sc_signal<bool> pcie_sii_reset_ctrl_;
    sc_core::sc_signal<bool> pcie_reset_ctrl_;
    
    void wire_components();
};

} // namespace pcie
} // namespace keraunos

#endif
