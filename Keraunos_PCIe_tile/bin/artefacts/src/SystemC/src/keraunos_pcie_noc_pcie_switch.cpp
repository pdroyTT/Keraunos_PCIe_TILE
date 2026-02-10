#include "keraunos_pcie_noc_pcie_switch.h"
#include <fstream>
#include <cstdio>

namespace keraunos {
namespace pcie {

NocPcieSwitch::NocPcieSwitch()
    : isolate_req_(false), pcie_outbound_enable_(true), pcie_inbound_enable_(true), system_ready_(true)
    , next_request_id_(1)
{}

void NocPcieSwitch::route_from_pcie(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    bool is_read = (trans.get_command() == tlm::TLM_READ_COMMAND);
    
    // #region agent log
    {std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug.log",std::ios::app);f<<"{\"location\":\"keraunos_pcie_noc_pcie_switch.cpp:11\",\"message\":\"PCIe transaction entry\",\"data\":{\"addr\":\"0x"<<std::hex<<addr<<std::dec<<"\",\"is_read\":"<<(is_read?"true":"false")<<",\"isolate\":"<<(isolate_req_?"true":"false")<<",\"inbound_en\":"<<(pcie_inbound_enable_?"true":"false")<<"},\"timestamp\":"<<sc_core::sc_time_stamp().value()<<",\"hypothesisId\":\"E\"}\n";}
    // #endregion
    
    // Check status register FIRST - this is a bypass path that works even when enables are off
    if (is_status_register_access(addr, is_read)) {
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        *data_ptr = get_status_reg_value();
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }
    
    // Now check enable bits for normal application traffic
    if (isolate_req_ || !pcie_inbound_enable_) {
        // #region agent log
        {std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug.log",std::ios::app);f<<"{\"location\":\"keraunos_pcie_noc_pcie_switch.cpp:25\",\"message\":\"Transaction blocked by isolation/enable\",\"data\":{\"isolate\":"<<(isolate_req_?"true":"false")<<",\"inbound_en\":"<<(pcie_inbound_enable_?"true":"false")<<"},\"timestamp\":"<<sc_core::sc_time_stamp().value()<<",\"hypothesisId\":\"E\"}\n";}
        // #endregion
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    NocPcieRoute route = route_address(addr, is_read);
    
    // CRITICAL FIX: Strip routing bits [63:60] before forwarding to TLBs
    // The TLB index calculation and address translation should operate on the
    // lower 60 bits (the actual PCIe address space), not the routing domain bits.
    uint64_t data_addr = addr & 0x0FFFFFFFFFFFFFFFULL;
    trans.set_address(data_addr);
    
    // #region agent log
    {std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug.log",std::ios::app);f<<"{\"location\":\"keraunos_pcie_noc_pcie_switch.cpp:route\",\"message\":\"Route extracted\",\"data\":{\"route\":"<<static_cast<int>(route)<<",\"route_bits\":"<<((addr>>60)&0xF)<<",\"data_addr\":\"0x"<<std::hex<<data_addr<<std::dec<<"\"},\"timestamp\":"<<sc_core::sc_time_stamp().value()<<",\"hypothesisId\":\"D\"}\n";}
    // #endregion
    
    switch(route) {
        case NocPcieRoute::TLB_APP_0:
            if (tlb_app_inbound0_) tlb_app_inbound0_(trans, delay);
            else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            break;
        case NocPcieRoute::TLB_APP_1:
            if (tlb_app_inbound1_) tlb_app_inbound1_(trans, delay);
            else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            break;
        case NocPcieRoute::TLB_SYS:
            if (tlb_sys_inbound_) tlb_sys_inbound_(trans, delay);
            else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            break;
        case NocPcieRoute::BYPASS_APP:
            if (noc_io_) noc_io_(trans, delay);
            else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            break;
        case NocPcieRoute::BYPASS_SYS:
            if (smn_io_) smn_io_(trans, delay);
            else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            break;
        default:
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            break;
    }
    
    // Restore original address with routing bits
    trans.set_address(addr);
    
    // If still incomplete, set OK as default
    if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void NocPcieSwitch::route_to_pcie(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Check outbound enable for application paths (matches inbound behavior)
    if (isolate_req_ || !pcie_outbound_enable_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    if (pcie_controller_) {
        pcie_controller_(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

NocPcieRoute NocPcieSwitch::route_address(uint64_t addr, bool is_read) const {
    uint8_t route_bits = (addr >> 60) & 0xF;
    switch(route_bits) {
        case 0: return NocPcieRoute::TLB_APP_0;
        case 1: return NocPcieRoute::TLB_APP_1;
        case 4: return NocPcieRoute::TLB_SYS;
        case 8: return NocPcieRoute::BYPASS_APP;
        case 9: return NocPcieRoute::BYPASS_SYS;
        default: return NocPcieRoute::DECERR_2;
    }
}

bool NocPcieSwitch::is_status_register_access(uint64_t addr, bool is_read) const {
    uint8_t route_bits = (addr >> 60) & 0xF;
    return is_read && (route_bits == 0xE || route_bits == 0xF) && system_ready_;
}

} // namespace pcie
} // namespace keraunos
