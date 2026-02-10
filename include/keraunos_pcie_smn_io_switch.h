#ifndef KERAUNOS_PCIE_SMN_IO_SWITCH_H
#define KERAUNOS_PCIE_SMN_IO_SWITCH_H

#include "keraunos_pcie_common.h"
#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <map>

namespace keraunos {
namespace pcie {

/**
 * SMN-IO Switch
 * 
 * Routes AXI4 transactions for System Management Network
 * - 64-bit data width
 * - 52-bit address width (masked to 64-bit for SCML2 compatibility)
 * - No read/write split
 * - Handles timeout and isolation
 */
class SmnIoSwitch : public sc_core::sc_module {
public:
    // Initiator ports (for sending transactions)
    scml2::initiator_socket<64> tlb_sys_inbound_port;   // To TLB Sys Inbound
    scml2::initiator_socket<64> tlb_sys_outbound_port;   // To TLB Sys Outbound
    scml2::initiator_socket<64> smn_n_initiator;        // To external SMN-N (outbound)
    
    // Target ports (for receiving transactions)
    tlm_utils::simple_target_socket<SmnIoSwitch, 64> smn_n_port;     // From external SMN-N / NOC-PCIE switch (inbound)
    
    // Initiator ports for routing to other components (32-bit)
    scml2::initiator_socket<32> msi_relay_cfg_port;        // To MSI Relay Config
    scml2::initiator_socket<32> sii_config_port;            // To SII Config
    scml2::initiator_socket<32> serdes_apb_port;            // To SerDes APB
    scml2::initiator_socket<32> serdes_ahb_port;           // To SerDes AHB
    
    // Initiator ports for TLB configuration (route to individual TLB config sockets)
    scml2::initiator_socket<32> tlb_sys_in0_cfg_initiator;     // To TLB Sys In0 config_socket
    scml2::initiator_socket<32> tlb_app_in0_cfg_initiator[4];  // To TLB App In0 config_socket (4 instances)
    scml2::initiator_socket<32> tlb_app_in1_cfg_initiator;     // To TLB App In1 config_socket
    scml2::initiator_socket<32> tlb_sys_out0_cfg_initiator;    // To TLB Sys Out0 config_socket
    scml2::initiator_socket<32> tlb_app_out0_cfg_initiator;     // To TLB App Out0 config_socket
    scml2::initiator_socket<32> tlb_app_out1_cfg_initiator;    // To TLB App Out1 config_socket
    
    // Control signals
    sc_core::sc_in<bool> isolate_req;                    // Isolation request
    
    // Timeout signal
    sc_core::sc_out<bool> timeout;                       // Timeout signal
    
    SC_HAS_PROCESS(SmnIoSwitch);
    
    SmnIoSwitch(sc_core::sc_module_name name);
    virtual ~SmnIoSwitch();
    
protected:
    // TLM transport methods for smn_n_port target socket
    void smn_n_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int smn_n_transport_dbg(tlm::tlm_generic_payload& trans);
    
    // Routing logic
    scml2::initiator_socket<64>* route_address(uint64_t addr);
    
    // Outstanding request tracking
    std::map<uint64_t, OutstandingRequest> outstanding_requests_;
    uint64_t next_request_id_;
    
    // Timeout handling
    sc_core::sc_time timeout_duration_;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_SMN_IO_SWITCH_H

