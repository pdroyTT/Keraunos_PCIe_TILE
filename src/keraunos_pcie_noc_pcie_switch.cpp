#include "keraunos_pcie_noc_pcie_switch.h"
#include "keraunos_pcie_common.h"
#include <cstring>

namespace keraunos {
namespace pcie {

NocPcieSwitch::NocPcieSwitch(sc_core::sc_module_name name)
    : sc_module(name)
    , tlb_app_inbound_port0("tlb_app_inbound_port0")
    , tlb_app_inbound_port1("tlb_app_inbound_port1")
    , tlb_sys_inbound_port("tlb_sys_inbound_port")
    , bypass_app_port("bypass_app_port")
    , bypass_sys_port("bypass_sys_port")
    , pcie_controller_initiator("pcie_controller_initiator")
    , tlb_app_out0_initiator("tlb_app_out0_initiator")
    , tlb_app_out1_initiator("tlb_app_out1_initiator")
    , tlb_sys_out0_initiator("tlb_sys_out0_initiator")
    , noc_io_initiator("noc_io_initiator")
    , smn_io_initiator("smn_io_initiator")
    , pcie_controller_target("pcie_controller_target")
    , tlb_app_outbound_port("tlb_app_outbound_port")
    , tlb_sys_outbound_port("tlb_sys_outbound_port")
    , msi_relay_port("msi_relay_port")
    , config_reg_port("config_reg_port")
    , noc_io_port("noc_io_port")
    , smn_io_port("smn_io_port")
    , next_request_id_(1)
    , status_register_(0)  // System ready = 0 initially
{
    // Register transport callbacks for all simple_target_socket ports
    pcie_controller_target.register_b_transport(this, &NocPcieSwitch::pcie_controller_target_b_transport);
    pcie_controller_target.register_transport_dbg(this, &NocPcieSwitch::pcie_controller_target_transport_dbg);
    
    tlb_app_outbound_port.register_b_transport(this, &NocPcieSwitch::tlb_app_outbound_b_transport);
    tlb_app_outbound_port.register_transport_dbg(this, &NocPcieSwitch::tlb_app_outbound_transport_dbg);
    
    tlb_sys_outbound_port.register_b_transport(this, &NocPcieSwitch::tlb_sys_outbound_b_transport);
    tlb_sys_outbound_port.register_transport_dbg(this, &NocPcieSwitch::tlb_sys_outbound_transport_dbg);
    
    msi_relay_port.register_b_transport(this, &NocPcieSwitch::msi_relay_b_transport);
    msi_relay_port.register_transport_dbg(this, &NocPcieSwitch::msi_relay_transport_dbg);
    
    config_reg_port.register_b_transport(this, &NocPcieSwitch::config_reg_b_transport);
    config_reg_port.register_transport_dbg(this, &NocPcieSwitch::config_reg_transport_dbg);
    
    noc_io_port.register_b_transport(this, &NocPcieSwitch::noc_io_b_transport);
    noc_io_port.register_transport_dbg(this, &NocPcieSwitch::noc_io_transport_dbg);
    
    smn_io_port.register_b_transport(this, &NocPcieSwitch::smn_io_b_transport);
    smn_io_port.register_transport_dbg(this, &NocPcieSwitch::smn_io_transport_dbg);
}

NocPcieSwitch::~NocPcieSwitch() {
}

NocPcieRoute NocPcieSwitch::route_address(uint64_t addr, bool is_read) const {
    uint8_t route_bits = (addr >> 60) & 0xF;
    
    // Check for isolation
    if (isolate_req.read()) {
        return NocPcieRoute::DECERR_2;  // Return DECERR when isolated
    }
    
    // Special handling for Status Register (route 0xE)
    if (route_bits == 0xE) {
        // Check if this is a Status Register read access
        // AxADDR[59:7] == 0 for Status Register (128B region)
        if (is_read && ((addr >> 7) & 0x1FFFFFFFFFFFFFFULL) == 0) {
            return NocPcieRoute::STATUS_REG;
        }
        // Otherwise route to TLB Sys0
        return NocPcieRoute::TLB_SYS;
    }
    
    // Route 0xF also goes to Status Register
    if (route_bits == 0xF) {
        return NocPcieRoute::STATUS_REG_ALT;
    }
    
    // Standard routing based on [63:60]
    switch (route_bits) {
        case 0x0: return NocPcieRoute::TLB_APP_0;
        case 0x1: return NocPcieRoute::TLB_APP_1;
        case 0x2: return NocPcieRoute::DECERR_2;
        case 0x3: return NocPcieRoute::DECERR_3;
        case 0x4: return NocPcieRoute::TLB_SYS;
        case 0x5: return NocPcieRoute::DECERR_5;
        case 0x6: return NocPcieRoute::DECERR_6;
        case 0x7: return NocPcieRoute::DECERR_7;
        case 0x8: return NocPcieRoute::BYPASS_APP;
        case 0x9: return NocPcieRoute::BYPASS_SYS;
        case 0xA: return NocPcieRoute::DECERR_10;
        case 0xB: return NocPcieRoute::DECERR_11;
        case 0xC: return NocPcieRoute::DECERR_12;
        case 0xD: return NocPcieRoute::DECERR_13;
        default: return NocPcieRoute::DECERR_2;
    }
}

scml2::initiator_socket<64>* NocPcieSwitch::get_initiator_port(NocPcieRoute route) {
    switch (route) {
        case NocPcieRoute::TLB_APP_0:
        case NocPcieRoute::TLB_APP_1:
            // Both route to TLB App Inbound (port selection based on [63:60])
            return (route == NocPcieRoute::TLB_APP_0) ? 
                &tlb_app_inbound_port0 : &tlb_app_inbound_port1;
        case NocPcieRoute::TLB_SYS:
            return &tlb_sys_inbound_port;
        case NocPcieRoute::BYPASS_APP:
            return &bypass_app_port;
        case NocPcieRoute::BYPASS_SYS:
            return &bypass_sys_port;
        default:
            return nullptr;  // DECERR routes
    }
}

tlm::tlm_base_target_socket<64>* NocPcieSwitch::get_target_port(NocPcieRoute route) {
    // This is for outbound routing (from PCIe Controller)
    // Outbound always goes to PCIe Controller port (handled separately)
    return nullptr;
}

uint64_t NocPcieSwitch::convert_64_to_52_addr(uint64_t addr) const {
    // Convert 64-bit address to 52-bit by masking upper bits
    // Use common masking function for consistency
    return mask_52bit_address(addr);
}

uint64_t NocPcieSwitch::convert_52_to_64_addr(uint64_t addr, uint8_t route) const {
    // Convert 52-bit address to 64-bit by adding route bits
    // First mask to ensure only 52 bits, then add route bits
    uint64_t masked_addr = mask_52bit_address(addr);
    return ((static_cast<uint64_t>(route) << 60) | masked_addr);
}

bool NocPcieSwitch::is_status_register_access(uint64_t addr, bool is_read) const {
    uint8_t route_bits = (addr >> 60) & 0xF;
    if (route_bits == 0xE && is_read) {
        // Check AxADDR[59:7] == 0 (128B region)
        return ((addr >> 7) & 0x1FFFFFFFFFFFFFFULL) == 0;
    }
    return (route_bits == 0xF);
}

void NocPcieSwitch::handle_status_register(tlm::tlm_generic_payload& trans) {
    uint64_t addr = trans.get_address();
    
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        // Read System Ready register
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        *data_ptr = status_register_;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        // Write System Ready register
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        status_register_ = *data_ptr;
        status_reg_value.write(status_register_);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

tlm::tlm_sync_enum NocPcieSwitch::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    bool is_read = (trans.get_command() == tlm::TLM_READ_COMMAND);
    
    // Check inbound enable for inbound traffic
    if (!pcie_inbound_app_enable.read() && !is_status_register_access(addr, is_read)) {
        // Check if this is Status Register access (allowed even when disabled)
        uint8_t route_bits = (addr >> 60) & 0xF;
        if (route_bits != 0xE && route_bits != 0xF) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return tlm::TLM_COMPLETED;
        }
    }
    
    // Handle Status Register access
    if (is_status_register_access(addr, is_read)) {
        handle_status_register(trans);
        return tlm::TLM_COMPLETED;
    }
    
    // Route based on address
    NocPcieRoute route = route_address(addr, is_read);
    
    // Handle DECERR routes
    if (route >= NocPcieRoute::DECERR_2 && route <= NocPcieRoute::DECERR_13) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return tlm::TLM_COMPLETED;
    }
    
    // Get appropriate initiator port
    scml2::initiator_socket<64>* init_port = get_initiator_port(route);
    if (init_port) {
        // Forward transaction using scml2::initiator_socket's mappable_if interface
        // scml2::initiator_socket implements mappable_if which has b_transport
        init_port->b_transport(trans, delay);
        return tlm::TLM_COMPLETED;
    }
    
    // Default: return DECERR
    trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    return tlm::TLM_COMPLETED;
}

// Callback implementations for simple_target_socket ports
// These are stub implementations that acknowledge transactions
// In a real system, these would implement full routing logic

void NocPcieSwitch::pcie_controller_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Receives inbound traffic from PCIe Controller
    // Route based on AxADDR[63:60] to appropriate TLB
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int NocPcieSwitch::pcie_controller_target_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void NocPcieSwitch::tlb_app_outbound_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Receives from TLB App Outbound, forward to PCIe Controller
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int NocPcieSwitch::tlb_app_outbound_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void NocPcieSwitch::tlb_sys_outbound_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Receives from TLB Sys Outbound, forward to PCIe Controller
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int NocPcieSwitch::tlb_sys_outbound_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void NocPcieSwitch::msi_relay_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // MSI Relay access (32-bit)
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int NocPcieSwitch::msi_relay_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void NocPcieSwitch::config_reg_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Config Register access (32-bit)
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int NocPcieSwitch::config_reg_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void NocPcieSwitch::noc_io_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // NOC-IO switch access
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int NocPcieSwitch::noc_io_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void NocPcieSwitch::smn_io_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // SMN-IO switch access
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int NocPcieSwitch::smn_io_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

bool NocPcieSwitch::get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi) {
    // DMI not supported through switch
    return false;
}

} // namespace pcie
} // namespace keraunos

