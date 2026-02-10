#include "keraunos_pcie_noc_io_switch.h"
#include "keraunos_pcie_common.h"
#include <cstring>

namespace keraunos {
namespace pcie {

NocIoSwitch::NocIoSwitch(sc_core::sc_module_name name)
    : sc_module(name)
    , noc_n_initiator("noc_n_initiator")
    , noc_n_port("noc_n_port")
    , tlb_app_inbound_port("tlb_app_inbound_port")
    , next_request_id_(1)
    , timeout_duration_(sc_core::sc_time(1, sc_core::SC_MS))  // Default 1ms timeout
{
    // Register transport callbacks for target sockets
    noc_n_port.register_b_transport(this, &NocIoSwitch::noc_n_b_transport);
    noc_n_port.register_transport_dbg(this, &NocIoSwitch::noc_n_transport_dbg);
    
    tlb_app_inbound_port.register_b_transport(this, &NocIoSwitch::tlb_inbound_b_transport);
    tlb_app_inbound_port.register_transport_dbg(this, &NocIoSwitch::tlb_inbound_transport_dbg);
}

NocIoSwitch::~NocIoSwitch() {
}

bool NocIoSwitch::route_to_noc_n(uint64_t addr) const {
    // Route to NOC-N if address doesn't match local resources
    // MSI Relay: 0x18800000
    // TLB App Outbound: 0x18900000 or AxADDR[51:48] != 0
    // DECERR regions: 0x18A00000, 0x18C00000, 0x18E00000
    
    uint32_t addr_32 = static_cast<uint32_t>(addr & 0xFFFFFFFFULL);
    
    // Check for MSI Relay
    if ((addr_32 >= 0x18800000) && (addr_32 < 0x18900000)) {
        return false;  // Route to MSI Relay
    }
    
    // Check for TLB App Outbound
    if ((addr_32 >= 0x18900000) && (addr_32 < 0x18A00000)) {
        return false;  // Route to TLB App Outbound
    }
    
    // Check for DECERR regions
    if (((addr_32 >= 0x18A00000) && (addr_32 < 0x18C00000)) ||
        ((addr_32 >= 0x18C00000) && (addr_32 < 0x18E00000)) ||
        ((addr_32 >= 0x18E00000) && (addr_32 < 0x19000000))) {
        return false;  // Return DECERR
    }
    
    // Check AxADDR[51:48] for TLB App Outbound routing
    if ((addr >> 48) & 0xF) {
        return false;  // Route to TLB App Outbound
    }
    
    // Default: route to NOC-N
    return true;
}

void NocIoSwitch::noc_n_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    
    // Check isolation
    if (isolate_req.read()) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        timeout_write.write(true);
        return;
    }
    
    uint32_t addr_32 = static_cast<uint32_t>(addr & 0xFFFFFFFFULL);
    
    // MSI Relay: 0x18800000 - 0x18900000 (1MB)
    if ((addr_32 >= 0x18800000) && (addr_32 < 0x18900000)) {
        // Route to MSI Relay (32-bit port)
        msi_relay_port->b_transport(trans, delay);
        return;
    }
    
    // TLB App Outbound: 0x18900000 - 0x18A00000 (1MB) or AxADDR[51:48] != 0
    if (((addr_32 >= 0x18900000) && (addr_32 < 0x18A00000)) || ((addr >> 48) & 0xF)) {
        // Route to TLB App Outbound
        tlb_app_outbound_port->b_transport(trans, delay);
        return;
    }
    
    // DECERR regions
    if (((addr_32 >= 0x18A00000) && (addr_32 < 0x18C00000)) ||
        ((addr_32 >= 0x18C00000) && (addr_32 < 0x18E00000)) ||
        ((addr_32 >= 0x18E00000) && (addr_32 < 0x19000000))) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        timeout_write.write(true);
        return;
    }
    
    // Default: route to external NOC-N via initiator
    // Mask address to 52 bits before routing
    set_52bit_address(trans, addr);
    noc_n_initiator->b_transport(trans, delay);
}

void NocIoSwitch::tlb_inbound_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // This receives traffic from TLBs after address translation
    // Forward directly to external NOC-N initiator
    uint64_t addr = trans.get_address();
    
    // Mask to 52 bits before sending to external
    set_52bit_address(trans, addr);
    
    // Forward to external NOC-N
    noc_n_initiator->b_transport(trans, delay);
}

unsigned int NocIoSwitch::tlb_inbound_transport_dbg(tlm::tlm_generic_payload& trans) {
    // Debug transport for TLB inbound path
    return noc_n_initiator->transport_dbg(trans);
}

unsigned int NocIoSwitch::noc_n_transport_dbg(tlm::tlm_generic_payload& trans) {
    uint64_t addr = trans.get_address();
    uint32_t addr_32 = static_cast<uint32_t>(addr & 0xFFFFFFFFULL);
    
    // MSI Relay: 0x18800000 - 0x18900000
    if ((addr_32 >= 0x18800000) && (addr_32 < 0x18900000)) {
        return msi_relay_port->transport_dbg(trans);
    }
    
    // TLB App Outbound: 0x18900000 - 0x18A00000 or AxADDR[51:48] != 0
    if (((addr_32 >= 0x18900000) && (addr_32 < 0x18A00000)) || ((addr >> 48) & 0xF)) {
        return tlb_app_outbound_port->transport_dbg(trans);
    }
    
    // DECERR regions
    if (((addr_32 >= 0x18A00000) && (addr_32 < 0x18C00000)) ||
        ((addr_32 >= 0x18C00000) && (addr_32 < 0x18E00000)) ||
        ((addr_32 >= 0x18E00000) && (addr_32 < 0x19000000))) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return 0;
    }
    
    // Default: route to external NOC-N via initiator
    return noc_n_initiator->transport_dbg(trans);
}

} // namespace pcie
} // namespace keraunos

