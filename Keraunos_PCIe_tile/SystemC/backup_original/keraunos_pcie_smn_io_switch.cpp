#include "keraunos_pcie_smn_io_switch.h"
#include "keraunos_pcie_common.h"
#include <cstring>

namespace keraunos {
namespace pcie {

SmnIoSwitch::SmnIoSwitch(sc_core::sc_module_name name)
    : sc_module(name)
    , smn_n_initiator("smn_n_initiator")
    , smn_n_port("smn_n_port")
    , next_request_id_(1)
    , timeout_duration_(sc_core::sc_time(1, sc_core::SC_MS))  // Default 1ms timeout
{
    // Register transport callbacks for smn_n_port target socket
    smn_n_port.register_b_transport(this, &SmnIoSwitch::smn_n_b_transport);
    smn_n_port.register_transport_dbg(this, &SmnIoSwitch::smn_n_transport_dbg);
}

SmnIoSwitch::~SmnIoSwitch() {
}

void SmnIoSwitch::smn_n_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    
    // Check isolation
    if (isolate_req.read()) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        timeout.write(true);
        return;
    }
    
    // Use existing route_address logic for 64-bit sockets
    auto* target = route_address(addr);
    
    if (target) {
        // target is a pointer to socket; dereference then use operator-> to access interface
        (*target)->b_transport(trans, delay);
    } else {
        // Handle 32-bit configuration ports
        uint32_t addr_32 = static_cast<uint32_t>(addr & 0xFFFFFFFFULL);
        
        // MSI Relay Config: 0x18000000
        if ((addr_32 >= 0x18000000) && (addr_32 < 0x18040000)) {
            msi_relay_cfg_port->b_transport(trans, delay);
            return;
        }
        
        // TLB Config: 0x18040000
        if ((addr_32 >= 0x18040000) && (addr_32 < 0x18050000)) {
            tlb_sys_in0_cfg_initiator->b_transport(trans, delay);
            return;
        }
        
        // SII Config: 0x18060000
        if ((addr_32 >= 0x18060000) && (addr_32 < 0x18080000)) {
            sii_config_port->b_transport(trans, delay);
            return;
        }
        
        // SerDes APB: 0x18080000
        if ((addr_32 >= 0x18080000) && (addr_32 < 0x18100000)) {
            serdes_apb_port->b_transport(trans, delay);
            return;
        }
        
        // SerDes AHB: 0x18100000
        if ((addr_32 >= 0x18100000) && (addr_32 < 0x18180000)) {
            serdes_ahb_port->b_transport(trans, delay);
            return;
        }
        
        // TLBSys0 Outbound Config: 0x18400000
        if ((addr_32 >= 0x18400000) && (addr_32 < 0x18500000)) {
            tlb_sys_out0_cfg_initiator->b_transport(trans, delay);
            return;
        }
        
        // DECERR regions
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        timeout.write(true);
    }
}

unsigned int SmnIoSwitch::smn_n_transport_dbg(tlm::tlm_generic_payload& trans) {
    // Debug transport - similar routing to b_transport
    uint64_t addr = trans.get_address();
    auto* target = route_address(addr);
    
    if (target) {
        // target is a pointer to socket; dereference then use operator-> to access interface
        return (*target)->transport_dbg(trans);
    }
    
    // Default: route to external SMN-N
    return smn_n_initiator->transport_dbg(trans);
}

tlm_utils::simple_initiator_socket<SmnIoSwitch, 64>* SmnIoSwitch::route_address(uint64_t addr) {
    // Mask address to 52 bits for routing logic
    addr = mask_52bit_address(addr);
    uint32_t addr_32 = static_cast<uint32_t>(addr & 0xFFFFFFFFULL);
    
    // MSI Relay Config: 0x18000000 (256KB, 8 PF x 16KB)
    // Note: 32-bit ports cannot be returned as 52-bit socket pointer
    // These will be handled via initiator ports instead
    if ((addr_32 >= 0x18000000) && (addr_32 < 0x18040000)) {
        return nullptr;  // Handle via initiator port
    }
    
    // TLB Config: 0x18040000 (64KB)
    if ((addr_32 >= 0x18040000) && (addr_32 < 0x18050000)) {
        return nullptr;  // Handle via initiator port
    }
    
    // SMN-IO Fabric CSR: 0x18050000 (64KB)
    if ((addr_32 >= 0x18050000) && (addr_32 < 0x18060000)) {
        // This is the switch's own CSR space
        return nullptr;  // Handle internally
    }
    
    // SII Config: 0x18060000 (128KB)
    if ((addr_32 >= 0x18060000) && (addr_32 < 0x18080000)) {
        return nullptr;  // Handle via initiator port (32-bit)
    }
    
    // SerDes APB: 0x18080000 (512KB)
    if ((addr_32 >= 0x18080000) && (addr_32 < 0x18100000)) {
        return nullptr;  // Handle via initiator port (32-bit)
    }
    
    // SerDes AHB: 0x18100000 (512KB)
    if ((addr_32 >= 0x18100000) && (addr_32 < 0x18180000)) {
        return nullptr;  // Handle via initiator port (32-bit)
    }
    
    // Reserved/DECERR: 0x18200000 - 0x18400000
    if ((addr_32 >= 0x18200000) && (addr_32 < 0x18400000)) {
        return nullptr;
    }
    
    // TLBSys0 Outbound: 0x18400000 - 0x18500000 (1MB)
    if ((addr_32 >= 0x18400000) && (addr_32 < 0x18500000)) {
        return &tlb_sys_outbound_port;
    }
    
    // Reserved/DECERR: 0x18500000 - 0x18800000
    if ((addr_32 >= 0x18500000) && (addr_32 < 0x18800000)) {
        return nullptr;  // Return DECERR
    }
    
    // Default: route to external SMN-N via initiator
    return &smn_n_initiator;
}

} // namespace pcie
} // namespace keraunos
