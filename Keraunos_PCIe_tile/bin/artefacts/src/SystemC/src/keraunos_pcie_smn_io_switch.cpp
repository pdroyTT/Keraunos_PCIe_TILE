#include "keraunos_pcie_smn_io_switch.h"

namespace keraunos {
namespace pcie {

SmnIoSwitch::SmnIoSwitch()
    : isolate_req_(false), timeout_(false), next_request_id_(1)
{}

void SmnIoSwitch::route_from_smn(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t addr = static_cast<uint32_t>(trans.get_address());
    
    if (isolate_req_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        timeout_ = true;
        return;
    }
    
    // ORIGINAL ADDRESS MAP per design spec (Appendix B.5)
    
    // MSI Relay Config: 0x18000000 - 0x1803FFFF (256KB, 8 PF × 16KB)
    if (addr >= 0x18000000 && addr < 0x18040000) {
        if (msi_relay_cfg_) {
            uint64_t offset = addr - 0x18000000;
            trans.set_address(offset);
            msi_relay_cfg_(trans, delay);
            trans.set_address(addr);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // Config Reg Block: 0x18040000 - 0x1804FFFF (64KB)
    // Contains TLB config space (B.1) and status registers (B.4)
    if (addr >= 0x18040000 && addr < 0x18050000) {
        uint32_t offset = addr - 0x18040000;
        
        // TLBSysOut0: offset 0x0000-0x0FFF (4KB, 16 entries × 64B)
        if (offset < 0x1000) {
            if (tlb_sys_out0_cfg_) {
                trans.set_address(offset);
                tlb_sys_out0_cfg_(trans, delay);
                trans.set_address(addr);
            } else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            return;
        }
        // TLBAppOut0: offset 0x1000-0x1FFF (4KB, 16 entries × 64B)
        if (offset < 0x2000) {
            if (tlb_app_out0_cfg_) {
                trans.set_address(offset - 0x1000);
                tlb_app_out0_cfg_(trans, delay);
                trans.set_address(addr);
            } else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            return;
        }
        // TLBAppOut1: offset 0x2000-0x2FFF (4KB, 16 entries × 64B)
        if (offset < 0x3000) {
            if (tlb_app_out1_cfg_) {
                trans.set_address(offset - 0x2000);
                tlb_app_out1_cfg_(trans, delay);
                trans.set_address(addr);
            } else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            return;
        }
        // TLBSysIn0: offset 0x3000-0x3FFF (4KB, 64 entries × 64B)
        if (offset < 0x4000) {
            if (tlb_sys_in0_cfg_) {
                trans.set_address(offset - 0x3000);
                tlb_sys_in0_cfg_(trans, delay);
                trans.set_address(addr);
            } else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            return;
        }
        // TLBAppIn0[0-3]: offset 0x4000-0x7FFF (4 instances × 4KB)
        if (offset < 0x8000) {
            int idx = (offset - 0x4000) >> 12;  // 0, 1, 2, or 3
            if (idx < 4 && tlb_app_in0_cfg_[idx]) {
                trans.set_address(offset - 0x4000 - (idx * 0x1000));
                tlb_app_in0_cfg_[idx](trans, delay);
                trans.set_address(addr);
            } else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            return;
        }
        // TLBAppIn1: offset 0x8000-0x8FFF (4KB, 64 entries × 64B)
        if (offset < 0x9000) {
            if (tlb_app_in1_cfg_) {
                trans.set_address(offset - 0x8000);
                tlb_app_in1_cfg_(trans, delay);
                trans.set_address(addr);
            } else trans.set_response_status(tlm::TLM_OK_RESPONSE);
            return;
        }
        // Remaining config space (status registers at 0xFFF8, 0xFFFC, etc.)
        if (config_reg_) {
            trans.set_address(offset);
            config_reg_(trans, delay);
            trans.set_address(addr);
        } else trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }
    
    // SMN-IO Fabric CSR: 0x18050000 - 0x1805FFFF (64KB)
    if (addr >= 0x18050000 && addr < 0x18060000) {
        // Placeholder for SMN-IO CSR access
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }
    
    // Gap: 0x18060000 - 0x1807FFFF (128KB, reserved)
    if (addr >= 0x18060000 && addr < 0x18080000) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    // SerDes AHB0: 0x18080000 - 0x180BFFFF (256KB)
    if (addr >= 0x18080000 && addr < 0x180C0000) {
        if (serdes_ahb_) {
            serdes_ahb_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // SerDes APB0: 0x180C0000 - 0x180FFFFF (256KB)
    if (addr >= 0x180C0000 && addr < 0x18100000) {
        if (serdes_apb_) {
            serdes_apb_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // SII Config: 0x18100000 - 0x181FFFFF (1MB, APB Demux)
    if (addr >= 0x18100000 && addr < 0x18200000) {
        if (sii_config_) {
            uint64_t offset = addr - 0x18100000;
            trans.set_address(offset);
            sii_config_(trans, delay);
            trans.set_address(addr);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // DECERR: 0x18200000 - 0x183FFFFF (2MB, reserved)
    if (addr >= 0x18200000 && addr < 0x18400000) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    // TLB Sys0 Outbound data path: 0x18400000 - 0x184FFFFF (1MB)
    if (addr >= 0x18400000 && addr < 0x18500000) {
        if (tlb_sys_outbound_) {
            tlb_sys_outbound_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    // DECERR: 0x18500000 - 0x187FFFFF (3MB, reserved)
    if (addr >= 0x18500000 && addr < 0x18800000) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    
    // Default: external SMN or OK
    if (smn_n_output_) {
        smn_n_output_(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void SmnIoSwitch::route_from_noc_io(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Same isolation logic
    if (isolate_req_) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        timeout_ = true;
        return;
    }
    
    // Route NOC-IO → MSI Relay data path
    if (msi_relay_data_) {
        msi_relay_data_(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void SmnIoSwitch::set_isolation(bool isolate) {
    isolate_req_ = isolate;
    if (!isolate) {
        timeout_ = false;
    }
}

bool SmnIoSwitch::get_timeout_status() const {
    return timeout_;
}

} // namespace pcie
} // namespace keraunos
