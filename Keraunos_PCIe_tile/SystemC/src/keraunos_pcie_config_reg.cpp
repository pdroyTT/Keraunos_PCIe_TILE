#include "keraunos_pcie_config_reg.h"
#include <cstring>

namespace keraunos {
namespace pcie {

ConfigRegBlock::ConfigRegBlock()
    : system_ready_(true)
    , pcie_outbound_app_enable_(true)
    , pcie_inbound_app_enable_(true)
    , isolate_req_(false)
    , config_memory_("config_memory", 64 * 1024)  // 64KB with SCML2 memory
    , change_callback_(nullptr)
{
    // Initialize registers with default values using array notation
    config_memory_[SYSTEM_READY_OFFSET] = 1;  // system_ready = true
    config_memory_[PCIE_ENABLE_OFFSET] = 1;   // outbound enable
    config_memory_[PCIE_ENABLE_OFFSET + 2] = 1;  // inbound enable (bit 16)
}

void ConfigRegBlock::process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        process_read(trans, delay);
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        process_write(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void ConfigRegBlock::set_isolate_req(const bool isolate) noexcept {
    isolate_req_ = isolate;
    if (isolate) {
        system_ready_ = false;
        pcie_outbound_app_enable_ = false;
        pcie_inbound_app_enable_ = false;
        // Notify about config change
        if (change_callback_) {
            change_callback_();
        }
    }
}

void ConfigRegBlock::process_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    // Read from SCML2 memory using subscript operator
    if (offset + len <= config_memory_.get_size()) {
        for (uint32_t i = 0; i < len; i++) {
            data_ptr[i] = config_memory_[offset + i];
        }
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        
        // Update from internal state for control registers
        if (offset == SYSTEM_READY_OFFSET && len >= 4) {
            uint32_t* val_ptr = reinterpret_cast<uint32_t*>(data_ptr);
            *val_ptr = system_ready_ ? 1 : 0;
        } else if (offset == PCIE_ENABLE_OFFSET && len >= 4) {
            uint32_t* val_ptr = reinterpret_cast<uint32_t*>(data_ptr);
            *val_ptr = (pcie_outbound_app_enable_ ? 0x1 : 0) | (pcie_inbound_app_enable_ ? 0x10000 : 0);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

void ConfigRegBlock::process_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    
    // Write to SCML2 memory using subscript operator
    if (offset + len <= config_memory_.get_size()) {
        for (uint32_t i = 0; i < len; i++) {
            config_memory_[offset + i] = data_ptr[i];
        }
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        
        // Update internal state for control registers
        bool config_changed = false;
        if (offset == SYSTEM_READY_OFFSET && len >= 4) {
            uint32_t* val_ptr = reinterpret_cast<uint32_t*>(data_ptr);
            system_ready_ = (*val_ptr & 0x1) != 0;
            config_changed = true;
        } else if (offset == PCIE_ENABLE_OFFSET && len >= 4) {
            uint32_t* val_ptr = reinterpret_cast<uint32_t*>(data_ptr);
            pcie_outbound_app_enable_ = (*val_ptr & 0x1) != 0;
            pcie_inbound_app_enable_ = (*val_ptr & 0x10000) != 0;
            config_changed = true;
        }
        
        // Notify parent module about config change via callback
        if (config_changed && change_callback_) {
            change_callback_();
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

} // namespace pcie
} // namespace keraunos
