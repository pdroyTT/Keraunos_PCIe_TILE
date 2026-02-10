#ifndef KERAUNOS_PCIE_NOC_PCIE_SWITCH_H
#define KERAUNOS_PCIE_NOC_PCIE_SWITCH_H

#include "keraunos_pcie_common.h"
#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <sc_dt.h>
#include <vector>
#include <map>

namespace keraunos {
namespace pcie {

/**
 * NOC-PCIE Switch
 * 
 * Routes AXI4 transactions based on AxADDR[63:60]
 * - 256-bit data width
 * - Supports read/write split
 * - Handles isolation and timeout
 * - Special routing for Status Register
 */
class NocPcieSwitch : public sc_core::sc_module {
public:
    // Initiator ports (from PCIe Controller and TLBs - inbound path)
    scml2::initiator_socket<64> tlb_app_inbound_port0;  // TLB App0 (AxADDR[63:60] = 0)
    scml2::initiator_socket<64> tlb_app_inbound_port1;  // TLB App1 (AxADDR[63:60] = 1)
    scml2::initiator_socket<64> tlb_sys_inbound_port;   // TLB Sys0 (AxADDR[63:60] = 4)
    scml2::initiator_socket<64> bypass_app_port;         // Bypass App (AxADDR[63:60] = 8)
    scml2::initiator_socket<64> bypass_sys_port;         // Bypass Sys (AxADDR[63:60] = 9)
    scml2::initiator_socket<64> pcie_controller_initiator;  // PCIe Controller (outbound - sending TO controller)
    
    // Initiator ports for outbound TLB connections (forward transactions to TLBs)
    scml2::initiator_socket<64> tlb_app_out0_initiator;        // To TLB App Out0 outbound_socket
    scml2::initiator_socket<64> tlb_app_out1_initiator;        // To TLB App Out1 outbound_socket
    scml2::initiator_socket<64> tlb_sys_out0_initiator;        // To TLB Sys Out0 outbound_socket
    
    // Initiator ports for switch interconnections
    scml2::initiator_socket<64> noc_io_initiator;               // To NOC-IO switch
    scml2::initiator_socket<64> smn_io_initiator;               // To SMN-IO switch
    
    // Target ports (to TLBs and external networks) - using simple_target_socket for auto-interface
    tlm_utils::simple_target_socket<NocPcieSwitch, 64> pcie_controller_target;    // PCIe Controller (inbound - receiving FROM controller)
    tlm_utils::simple_target_socket<NocPcieSwitch, 64> tlb_app_outbound_port;     // TLB App Outbound (receives from PCIe Controller)
    tlm_utils::simple_target_socket<NocPcieSwitch, 64> tlb_sys_outbound_port;      // TLB Sys Outbound (receives from PCIe Controller)
    tlm_utils::simple_target_socket<NocPcieSwitch, 32> msi_relay_port;             // MSI Relay MSI port
    tlm_utils::simple_target_socket<NocPcieSwitch, 32> config_reg_port;            // Config Register
    tlm_utils::simple_target_socket<NocPcieSwitch, 64> noc_io_port;                // NOC-IO (52-bit address masked to 64-bit)
    tlm_utils::simple_target_socket<NocPcieSwitch, 64> smn_io_port;                // SMN-IO (52-bit address masked to 64-bit)
    
    // Control signals
    sc_core::sc_in<bool> isolate_req;                    // Isolation request
    sc_core::sc_in<bool> pcie_outbound_app_enable;       // Outbound enable
    sc_core::sc_in<bool> pcie_inbound_app_enable;        // Inbound enable
    sc_core::sc_in<bool> system_ready;                   // System ready bit
    
    // Status Register (special handling)
    sc_core::sc_out<uint32_t> status_reg_value;         // System Ready register value
    
    SC_HAS_PROCESS(NocPcieSwitch);
    
    NocPcieSwitch(sc_core::sc_module_name name);
    virtual ~NocPcieSwitch();
    
protected:
    // TLM transport callback methods for simple_target_socket ports
    void pcie_controller_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int pcie_controller_target_transport_dbg(tlm::tlm_generic_payload& trans);
    void tlb_app_outbound_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int tlb_app_outbound_transport_dbg(tlm::tlm_generic_payload& trans);
    void tlb_sys_outbound_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int tlb_sys_outbound_transport_dbg(tlm::tlm_generic_payload& trans);
    void msi_relay_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int msi_relay_transport_dbg(tlm::tlm_generic_payload& trans);
    void config_reg_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int config_reg_transport_dbg(tlm::tlm_generic_payload& trans);
    void noc_io_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int noc_io_transport_dbg(tlm::tlm_generic_payload& trans);
    void smn_io_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int smn_io_transport_dbg(tlm::tlm_generic_payload& trans);
    
    // Routing logic
    NocPcieRoute route_address(uint64_t addr, bool is_read) const;
    scml2::initiator_socket<64>* get_initiator_port(NocPcieRoute route);
    tlm::tlm_base_target_socket<64>* get_target_port(NocPcieRoute route);
    
    // Address conversion helpers
    uint64_t convert_64_to_52_addr(uint64_t addr) const;
    uint64_t convert_52_to_64_addr(uint64_t addr, uint8_t route) const;
    
    // Outstanding request tracking
    std::map<uint64_t, OutstandingRequest> outstanding_requests_;
    uint64_t next_request_id_;
    
    // Status register (System Ready)
    uint32_t status_register_;
    
    // Helper methods
    bool is_status_register_access(uint64_t addr, bool is_read) const;
    void handle_status_register(tlm::tlm_generic_payload& trans);
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_NOC_PCIE_SWITCH_H

