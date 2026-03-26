#include "keraunos_pcie_config_reg.h"
#include <cstring>

namespace keraunos {
namespace pcie {

ConfigRegBlock::ConfigRegBlock()
    : system_ready_(true)
    , pcie_outbound_app_enable_(true)
    , pcie_inbound_app_enable_(true)
    , isolate_req_(false)
    , debug_logging_enabled_(true)
    , parent_module_(nullptr)
    , config_memory_("config_memory", 64 * 1024)
    , change_callback_(nullptr)
{
    config_memory_[SYSTEM_READY_OFFSET] = 1;
    config_memory_[PCIE_ENABLE_OFFSET] = 1;
    config_memory_[PCIE_ENABLE_OFFSET + 2] = 1;
}

void ConfigRegBlock::process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        process_read(trans, delay);
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        process_write(trans, delay);
    } else {
        if (parent_module_) {
            SCML2_ERROR_TO(parent_module_, FUNCTIONAL_ERROR) 
                << "Invalid APB command type: " << trans.get_command() << std::endl;
        }
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void ConfigRegBlock::set_isolate_req(const bool isolate) noexcept {
    if (isolate_req_ != isolate) {
        isolate_req_ = isolate;
        if (isolate) {
            if (debug_logging_enabled_ && parent_module_) {
                SCML2_WARNING_TO(parent_module_, CONFIGURATION_WARNING) 
                    << "Isolation requested - disabling system_ready and PCIe enables" << std::endl;
            }
            system_ready_ = false;
            pcie_outbound_app_enable_ = false;
            pcie_inbound_app_enable_ = false;
            
            if (change_callback_) {
                change_callback_();
            }
        }
    }
}

void ConfigRegBlock::process_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    if (debug_logging_enabled_ && parent_module_) {
        SCML2_INFO_TO(parent_module_, FUNCTIONAL_LOG) 
            << "Config register read: offset=0x" << std::hex << offset << std::dec 
            << " len=" << len << std::endl;
    }
    
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
            if (debug_logging_enabled_ && parent_module_) {
                SCML2_INFO_TO(parent_module_, FUNCTIONAL_LOG) 
                    << "SYSTEM_READY read: " << system_ready_ << std::endl;
            }
        } else if (offset == PCIE_ENABLE_OFFSET && len >= 4) {
            uint32_t* val_ptr = reinterpret_cast<uint32_t*>(data_ptr);
            *val_ptr = (pcie_outbound_app_enable_ ? 0x1 : 0) | (pcie_inbound_app_enable_ ? 0x10000 : 0);
            if (debug_logging_enabled_ && parent_module_) {
                SCML2_INFO_TO(parent_module_, FUNCTIONAL_LOG) 
                    << "PCIE_ENABLE read: outbound=" << pcie_outbound_app_enable_ 
                    << " inbound=" << pcie_inbound_app_enable_ << std::endl;
            }
        }
    } else {
        if (parent_module_) {
            SCML2_ERROR_TO(parent_module_, ACCESS_UNMAPPED_ADDRESS) 
                << "Config read to unmapped address: offset=0x" << std::hex << offset << std::dec 
                << " len=" << len << std::endl;
        }
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

void ConfigRegBlock::process_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    if (debug_logging_enabled_ && parent_module_) {
        SCML2_INFO_TO(parent_module_, FUNCTIONAL_LOG) 
            << "Config register write: offset=0x" << std::hex << offset << std::dec 
            << " len=" << len << std::endl;
    }
    
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
            bool new_val = (*val_ptr & 0x1) != 0;
            if (system_ready_ != new_val) {
                system_ready_ = new_val;
                config_changed = true;
                if (debug_logging_enabled_ && parent_module_) {
                    SCML2_INFO_TO(parent_module_, CONFIGURATION_INFO) 
                        << "SYSTEM_READY updated to " << system_ready_ << std::endl;
                }
            }
        } else if (offset == PCIE_ENABLE_OFFSET && len >= 4) {
            uint32_t* val_ptr = reinterpret_cast<uint32_t*>(data_ptr);
            bool new_out = (*val_ptr & 0x1) != 0;
            bool new_in = (*val_ptr & 0x10000) != 0;
            if (pcie_outbound_app_enable_ != new_out || pcie_inbound_app_enable_ != new_in) {
                pcie_outbound_app_enable_ = new_out;
                pcie_inbound_app_enable_ = new_in;
                config_changed = true;
                if (debug_logging_enabled_ && parent_module_) {
                    SCML2_INFO_TO(parent_module_, CONFIGURATION_INFO) 
                        << "PCIE_ENABLE updated: outbound=" << pcie_outbound_app_enable_ 
                        << " inbound=" << pcie_inbound_app_enable_ << std::endl;
                }
            }
        }
        
        // Notify parent module about config change via callback
        if (config_changed && change_callback_) {
            change_callback_();
        }
    } else {
        if (parent_module_) {
            SCML2_ERROR_TO(parent_module_, ACCESS_UNMAPPED_ADDRESS) 
                << "Config write to unmapped address: offset=0x" << std::hex << offset << std::dec 
                << " len=" << len << std::endl;
        }
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

} // namespace pcie
} // namespace keraunos
