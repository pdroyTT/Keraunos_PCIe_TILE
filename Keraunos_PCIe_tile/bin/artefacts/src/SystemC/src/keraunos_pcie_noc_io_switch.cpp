#include "keraunos_pcie_noc_io_switch.h"
#include <cstring>

namespace keraunos {
namespace pcie {

NocIoSwitch::NocIoSwitch()
    : isolate_req_(false), timeout_read_(false), timeout_write_(false)
    , next_request_id_(1)
{}

void NocIoSwitch::route_from_noc(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    if (isolate_req_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        timeout_write_ = true;
        return;
    }
    
    uint64_t addr = trans.get_address();
    uint32_t addr_32 = static_cast<uint32_t>(addr & 0xFFFFFFFFULL);
    
    // MSI Relay: 0x18800000 - 0x18900000
    if ((addr_32 >= 0x18800000) && (addr_32 < 0x18900000)) {
        if (msi_relay_output_) {
            uint64_t offset = addr_32 - 0x18800000;
            trans.set_address(offset);
            msi_relay_output_(trans, delay);
            trans.set_address(addr);  // Restore
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // TLB App Outbound: 0x18900000 - 0x18A00000
    if ((addr_32 >= 0x18900000) && (addr_32 < 0x18A00000)) {
        if (tlb_app_output_) {
            tlb_app_output_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // DECERR regions
    if (((addr_32 >= 0x18A00000) && (addr_32 < 0x18C00000)) ||
        ((addr_32 >= 0x18C00000) && (addr_32 < 0x18E00000)) ||
        ((addr_32 >= 0x18E00000) && (addr_32 < 0x19000000))) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    // Check AxADDR[51:48] for TLB routing
    if ((addr >> 48) & 0xF) {
        if (tlb_app_output_) {
            tlb_app_output_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // Default: route to NOC-N or set OK if no output connected
    if (noc_n_output_) {
        noc_n_output_(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void NocIoSwitch::route_from_tlb(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // From TLB, always route to external NOC-N
    if (noc_n_output_) {
        noc_n_output_(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

bool NocIoSwitch::route_to_noc_n(uint64_t addr) const {
    uint32_t addr_32 = static_cast<uint32_t>(addr & 0xFFFFFFFFULL);
    if ((addr_32 >= 0x18800000) && (addr_32 < 0x19000000)) return false;
    if ((addr >> 48) & 0xF) return false;
    return true;
}

} // namespace pcie
} // namespace keraunos
