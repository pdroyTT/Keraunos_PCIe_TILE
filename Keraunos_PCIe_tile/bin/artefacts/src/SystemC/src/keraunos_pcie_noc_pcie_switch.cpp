#include "keraunos_pcie_noc_pcie_switch.h"
#include <fstream>
#include <cstdio>

namespace keraunos {
namespace pcie {

NocPcieSwitch::NocPcieSwitch()
    : isolate_req_(false), pcie_outbound_enable_(true), pcie_inbound_enable_(true), system_ready_(true)
    , bus_master_enable_(true), controller_is_ep_(true)  // Keraunos is EP-only (Table 6)
    , next_request_id_(1)
{}

void NocPcieSwitch::route_from_pcie(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    bool is_read = (trans.get_command() == tlm::TLM_READ_COMMAND);
    
    // Step 1: Isolation blocks ALL traffic (physical AXI tie-off per Section 2.2.1.5)
    if (isolate_req_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    // Step 2: Status register access bypasses inbound enable check
    // Per Section 2.5.8.2 (Example 1): route 0xF always allowed,
    // route 0xE allowed when AxADDR[59:7]==0 && read.
    if (is_status_register_access(addr, is_read)) {
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        *data_ptr = get_status_reg_value();
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }
    
    // Step 3: Check inbound enable for normal application traffic
    if (!pcie_inbound_enable_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    NocPcieRoute route = route_address(addr, is_read);
    
    // Strip routing bits [63:60] before forwarding to TLBs
    uint64_t data_addr = addr & 0x0FFFFFFFFFFFFFFFULL;
    trans.set_address(data_addr);
    
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
            // Bypass path requires system_ready (Section 2.3.1)
            if (!system_ready_) {
                trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            } else if (noc_io_) {
                noc_io_(trans, delay);
            } else {
                trans.set_response_status(tlm::TLM_OK_RESPONSE);
            }
            break;
        case NocPcieRoute::BYPASS_SYS:
            // Bypass path requires system_ready (Section 2.3.1)
            if (!system_ready_) {
                trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            } else if (smn_io_) {
                smn_io_(trans, delay);
            } else {
                trans.set_response_status(tlm::TLM_OK_RESPONSE);
            }
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
    // No AxUSER provided — delegate with zero AxUSER (treated as memory TLP for BME)
    // #region agent log
    {
        std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug_bme.log", std::ios::app);
        f << "route_to_pcie 2-arg called (NO axuser) addr=0x" << std::hex << trans.get_address() << std::dec << "\n";
    }
    // #endregion
    sc_dt::sc_bv<256> zero_axuser(0);
    route_to_pcie(trans, delay, zero_axuser);
}

void NocPcieSwitch::route_to_pcie(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay,
                                   const sc_dt::sc_bv<256>& axuser) {
    // Step 1: Isolation blocks all outbound traffic (Section 2.2.1.5)
    if (isolate_req_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }

    // Step 2: Outbound enable check (Table 33, rows with outbound_enable=0)
    if (!pcie_outbound_enable_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }

    // Step 3: Bus Master Enable qualification (Table 33, Section 2.5.8.1)
    //  - RP mode: BME is not checked (all traffic passes when outbound=1)
    //  - EP mode, BME=1: all traffic passes
    //  - EP mode, BME=0: only BME-exempt TLPs pass (Cfg, Msg, DBI); Mem TLPs get DECERR
    if (controller_is_ep_ && !bus_master_enable_) {
        bool exempt = is_bme_exempt(axuser);
        // #region agent log
        {
            std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug_bme.log", std::ios::app);
            char buf[512];
            uint32_t tlp_type = 0;
            for (int i = 0; i < 5; i++) { if (axuser[i].to_bool()) tlp_type |= (1u << i); }
            bool dbi = axuser[21].to_bool();
            snprintf(buf, sizeof(buf),
                "route_to_pcie BME CHECK: ep=%d bme=%d exempt=%d tlp_type=%u dbi=%d addr=0x%lx cmd=%d\n",
                (int)controller_is_ep_, (int)bus_master_enable_, (int)exempt,
                tlp_type, (int)dbi, (unsigned long)trans.get_address(),
                (int)trans.get_command());
            f << buf;
        }
        // #endregion
        if (!exempt) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }
    }

    // Step 4: Forward to PCIe controller
    if (pcie_controller_) {
        pcie_controller_(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

bool NocPcieSwitch::is_bme_exempt(const sc_dt::sc_bv<256>& axuser) const {
    // Extract fields from subordinate AxUSER (Table 24/25):
    //   TLP Type [4:0]  — request type encoding
    //   DBI      [21]   — DBI Access Indicator
    uint32_t tlp_type = 0;
    for (int i = 0; i < 5; i++) {
        if (axuser[i].to_bool()) tlp_type |= (1u << i);
    }
    bool is_dbi = axuser[21].to_bool();

    // Table 34: TLP types NOT affected by BME
    //   CfgRd/Wr : TYPE[4:0] = 0010x  → decimal 4 or 5
    //   Msg/MsgD  : TYPE[4:0] = 10xxx  → decimal 16..23
    bool is_cfg = ((tlp_type >> 1) == 2);   // 0010x → {4,5} >> 1 == 2
    bool is_msg = ((tlp_type >> 3) == 2);   // 10xxx → {16..23} >> 3 == 2

    return is_dbi || is_cfg || is_msg;
}

NocPcieRoute NocPcieSwitch::route_address(uint64_t addr, bool is_read) const {
    uint8_t route_bits = (addr >> 60) & 0xF;
    switch(route_bits) {
        case 0:  return NocPcieRoute::TLB_APP_0;
        case 1:  return NocPcieRoute::TLB_APP_1;
        case 4:  return NocPcieRoute::TLB_SYS;
        case 8:  return NocPcieRoute::BYPASS_APP;
        case 9:  return NocPcieRoute::BYPASS_SYS;
        // Per Table 32: route 0xE non-status-register cases → TLB Sys0
        // (status register case already handled before route_address is called)
        case 14: return NocPcieRoute::TLB_SYS;
        default: return NocPcieRoute::DECERR_2;
    }
}

bool NocPcieSwitch::is_status_register_access(uint64_t addr, bool is_read) const {
    uint8_t route_bits = (addr >> 60) & 0xF;
    // Per Table 32 + Section 2.5.8.2 (Example 1):
    // Route 0xF: ALWAYS status register (read or write)
    if (route_bits == 0xF) return true;
    // Route 0xE: status register ONLY when AxADDR[59:7]==0 AND read
    if (route_bits == 0xE && is_read) {
        uint64_t addr_59_7 = (addr >> 7) & ((1ULL << 53) - 1);
        return (addr_59_7 == 0);
    }
    return false;
}

} // namespace pcie
} // namespace keraunos
