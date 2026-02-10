#ifndef KERAUNOS_PCIE_NOC_IO_SWITCH_H
#define KERAUNOS_PCIE_NOC_IO_SWITCH_H

#include "keraunos_pcie_common.h"
#include <scml2.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <sc_dt.h>
#include <map>

namespace keraunos {
namespace pcie {

/**
 * NOC-IO Switch
 * 
 * Routes AXI4 transactions for NOC interface
 * - 256-bit data width
 * - 52-bit address width (masked to 64-bit for SCML2 compatibility)
 * - Supports read/write split
 * - Handles timeout and isolation
 */
class NocIoSwitch : public sc_core::sc_module {
public:
    // Initiator ports (for sending transactions)
    tlm_utils::simple_initiator_socket<NocIoSwitch, 64> noc_n_initiator;        // To external NOC-N (outbound)
    tlm_utils::simple_initiator_socket<NocIoSwitch, 64> tlb_app_outbound_port;  // To TLB App Outbound
    tlm_utils::simple_initiator_socket<NocIoSwitch, 32> msi_relay_port;         // To MSI Relay
    
    // Target ports (for receiving transactions)
    tlm_utils::simple_target_socket<NocIoSwitch, 64> noc_n_port;              // From external NOC-N / NOC-PCIE switch (inbound)
    tlm_utils::simple_target_socket<NocIoSwitch, 64> tlb_app_inbound_port;    // From TLB App Inbound (after translation)
    
    // Control signals
    sc_core::sc_in<bool> isolate_req;                    // Isolation request
    
    // Timeout signals
    sc_core::sc_out<bool> timeout_read;                  // Read timeout
    sc_core::sc_out<bool> timeout_write;                // Write timeout
    
    SC_HAS_PROCESS(NocIoSwitch);
    
    NocIoSwitch(sc_core::sc_module_name name);
    virtual ~NocIoSwitch();
    
protected:
    // TLM transport methods for noc_n_port target socket
    void noc_n_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int noc_n_transport_dbg(tlm::tlm_generic_payload& trans);
    
    // TLM transport methods for tlb_app_inbound_port target socket
    void tlb_inbound_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int tlb_inbound_transport_dbg(tlm::tlm_generic_payload& trans);
    
    // Routing logic
    bool route_to_noc_n(uint64_t addr) const;
    
    // Outstanding request tracking
    std::map<uint64_t, OutstandingRequest> outstanding_requests_;
    uint64_t next_request_id_;
    
    // Timeout handling
    sc_core::sc_time timeout_duration_;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_NOC_IO_SWITCH_H

