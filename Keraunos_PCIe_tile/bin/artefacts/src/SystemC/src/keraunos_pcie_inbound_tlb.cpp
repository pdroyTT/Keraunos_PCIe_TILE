#include "keraunos_pcie_inbound_tlb.h"
#include <cstring>

namespace keraunos {
namespace pcie {

// TLBSysIn0
TLBSysIn0::TLBSysIn0() : entries_(64), system_ready_(true), tlb_memory_("tlb_sys_in0_memory", 4096) {
    // Initialize entry 0 as valid for basic testing
    entries_[0].valid = true;
    entries_[0].addr = 0x80000000 >> 12;  // Physical address
    entries_[0].attr = 0x100;
}

void TLBSysIn0::process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        if (offset + len <= tlb_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                data_ptr[i] = tlb_memory_[offset + i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        if (offset + len <= tlb_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                tlb_memory_[offset + i] = data_ptr[i];
            }
            
            // CRITICAL FIX: Parse memory writes and update entries_ vector
            uint8_t entry_index = offset / 64;
            uint32_t entry_offset = offset % 64;
            
            if (entry_index < entries_.size()) {
                if (entry_offset < 8) {
                    uint32_t lower = 0, upper = 0;
                    for (int b = 0; b < 4; b++) lower |= ((uint32_t)(uint8_t)tlb_memory_[entry_index * 64 + b]) << (b * 8);
                    for (int b = 0; b < 4; b++) upper |= ((uint32_t)(uint8_t)tlb_memory_[entry_index * 64 + 4 + b]) << (b * 8);
                    
                    entries_[entry_index].valid = (lower & 0x1) != 0;
                    entries_[entry_index].addr = ((uint64_t)upper << 32) | (lower & 0xFFFFF000);
                    entries_[entry_index].addr >>= 12;
                } else if (entry_offset == 32 && len == 4) {
                    uint32_t attr_val = *reinterpret_cast<uint32_t*>(data_ptr);
                    entries_[entry_index].attr = attr_val;
                }
            }
            
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void TLBSysIn0::process_inbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t iatu_addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser;
    
    if (lookup(iatu_addr, translated_addr, axuser)) {
        trans.set_address(translated_addr);
        if (translated_output_) {
            translated_output_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

bool TLBSysIn0::lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser) {
    // Note: system_ready does NOT gate TLB lookup (Section 2.3.1 spec).
    // Only bypass paths (route 8,9) are gated by system_ready.
    uint8_t index = calculate_index(iatu_addr);
    if (index >= entries_.size()) return false;
    const TlbEntry& entry = entries_[index];
    if (!entry.valid) return false;
    // Spec: translated = {ADDR[51:14], pa[13:0]}  (14-bit page size)
    constexpr uint64_t page_mask = (1ULL << 14) - 1;
    translated_addr = ((entry.addr << 12) & ~page_mask) | (iatu_addr & page_mask);
    // Spec DV note: 12-bit axuser = {ATTR[11:4], 2'b0, ATTR[1:0]}
    uint32_t raw_attr = entry.attr.to_uint();
    axuser = ((raw_attr >> 4) & 0xFF) << 4 | (raw_attr & 0x3);
    return true;
}

void TLBSysIn0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) entries_[index] = entry;
}

TlbEntry TLBSysIn0::get_entry(uint8_t index) const {
    return (index < entries_.size()) ? entries_[index] : TlbEntry();
}

uint8_t TLBSysIn0::calculate_index(uint64_t addr) const {
    return static_cast<uint8_t>((addr >> 14) & 0x3F);
}

// TLBAppIn0
TLBAppIn0::TLBAppIn0(uint8_t instance_id) 
    : instance_id_(instance_id), entries_(64)
    , tlb_memory_(std::string("tlb_app_in0_") + std::to_string(instance_id) + "_memory", 4096)
{
    // Initialize entry 0 as valid for basic testing
    entries_[0].valid = true;
    entries_[0].addr = 0x80000000 >> 12;
    entries_[0].attr = 0x100;
}

void TLBAppIn0::process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        if (offset + len <= tlb_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                data_ptr[i] = tlb_memory_[offset + i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        if (offset + len <= tlb_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                tlb_memory_[offset + i] = data_ptr[i];
            }
            
            // CRITICAL FIX: Parse memory writes and update entries_ vector
            // TLB entry format: 64 bytes per entry
            // offset 0: lower 32 bits (valid bit + addr[31:12])
            // offset 4: upper 32 bits (addr[63:32])
            // offset 32: attributes
            uint8_t entry_index = offset / 64;
            uint32_t entry_offset = offset % 64;
            
            if (entry_index < entries_.size()) {
                // Check if this write affects entry fields we care about
                if (entry_offset < 8) {
                    // Address fields being written - reconstruct full entry
                    uint32_t lower = 0, upper = 0;
                    for (int b = 0; b < 4; b++) lower |= ((uint32_t)(uint8_t)tlb_memory_[entry_index * 64 + b]) << (b * 8);
                    for (int b = 0; b < 4; b++) upper |= ((uint32_t)(uint8_t)tlb_memory_[entry_index * 64 + 4 + b]) << (b * 8);
                    
                    entries_[entry_index].valid = (lower & 0x1) != 0;
                    entries_[entry_index].addr = ((uint64_t)upper << 32) | (lower & 0xFFFFF000);
                    entries_[entry_index].addr >>= 12;  // Store as shifted value
                } else if (entry_offset == 32 && len == 4) {
                    // Attributes being written
                    uint32_t attr_val = *reinterpret_cast<uint32_t*>(data_ptr);
                    entries_[entry_index].attr = attr_val;
                }
            }
            
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void TLBAppIn0::process_inbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t iatu_addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser;
    
    if (lookup(iatu_addr, translated_addr, axuser)) {
        trans.set_address(translated_addr);
        if (translated_output_) {
            translated_output_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

bool TLBAppIn0::lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser) {
    uint8_t index = calculate_index(iatu_addr);
    if (index >= entries_.size()) return false;
    const TlbEntry& entry = entries_[index];
    if (!entry.valid) return false;
    // Spec: translated = {ADDR[51:24], pa[23:0]}  (24-bit page size / 16MB)
    constexpr uint64_t page_mask = (1ULL << 24) - 1;
    translated_addr = ((entry.addr << 12) & ~page_mask) | (iatu_addr & page_mask);
    // Spec DV note: 12-bit axuser = {3'b0, ATTR[4:0], 4'b0}
    uint32_t raw_attr = entry.attr.to_uint();
    axuser = (raw_attr & 0x1F) << 4;
    return true;
}

void TLBAppIn0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) {
        entries_[index] = entry;
        // #region agent log
        {std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug.log",std::ios::app);f<<"{\"location\":\"keraunos_pcie_inbound_tlb.cpp:150\",\"message\":\"TLB App In0 configured\",\"data\":{\"index\":"<<(int)index<<",\"addr\":\"0x"<<std::hex<<entry.addr<<std::dec<<"\",\"valid\":"<<(entry.valid?"true":"false")<<",\"attr\":"<<entry.attr.to_uint()<<"},\"timestamp\":"<<sc_core::sc_time_stamp().value()<<",\"hypothesisId\":\"B\"}\n";}
        // #endregion
    }
}

TlbEntry TLBAppIn0::get_entry(uint8_t index) const {
    return (index < entries_.size()) ? entries_[index] : TlbEntry();
}

uint8_t TLBAppIn0::calculate_index(uint64_t addr) const {
    return static_cast<uint8_t>((addr >> 24) & 0x3F);
}

// TLBAppIn1
TLBAppIn1::TLBAppIn1() : entries_(64), tlb_memory_("tlb_app_in1_memory", 4096) {
    // Initialize entry 0 as valid
    // Base must be 8GB-aligned (bits[63:33] populated) for 33-bit page TLB
    entries_[0].valid = true;
    entries_[0].addr = 0x200000000ULL >> 12;
    entries_[0].attr = 0x200;
}

void TLBAppIn1::process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        if (offset + len <= tlb_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                data_ptr[i] = tlb_memory_[offset + i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        if (offset + len <= tlb_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                tlb_memory_[offset + i] = data_ptr[i];
            }
            
            // CRITICAL FIX: Parse memory writes and update entries_ vector
            uint8_t entry_index = offset / 64;
            uint32_t entry_offset = offset % 64;
            
            if (entry_index < entries_.size()) {
                if (entry_offset < 8) {
                    uint32_t lower = 0, upper = 0;
                    for (int b = 0; b < 4; b++) lower |= ((uint32_t)(uint8_t)tlb_memory_[entry_index * 64 + b]) << (b * 8);
                    for (int b = 0; b < 4; b++) upper |= ((uint32_t)(uint8_t)tlb_memory_[entry_index * 64 + 4 + b]) << (b * 8);
                    
                    entries_[entry_index].valid = (lower & 0x1) != 0;
                    entries_[entry_index].addr = ((uint64_t)upper << 32) | (lower & 0xFFFFF000);
                    entries_[entry_index].addr >>= 12;
                } else if (entry_offset == 32 && len == 4) {
                    uint32_t attr_val = *reinterpret_cast<uint32_t*>(data_ptr);
                    entries_[entry_index].attr = attr_val;
                }
            }
            
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void TLBAppIn1::process_inbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t iatu_addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser;
    
    if (lookup(iatu_addr, translated_addr, axuser)) {
        trans.set_address(translated_addr);
        if (translated_output_) {
            translated_output_(trans, delay);
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

bool TLBAppIn1::lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser) {
    uint8_t index = calculate_index(iatu_addr);
    if (index >= entries_.size()) return false;
    const TlbEntry& entry = entries_[index];
    if (!entry.valid) return false;
    // Spec: translated = {ADDR[51:33], pa[32:0]}  (33-bit page size / 8GB)
    constexpr uint64_t page_mask = (1ULL << 33) - 1;
    translated_addr = ((entry.addr << 12) & ~page_mask) | (iatu_addr & page_mask);
    // Spec DV note: 12-bit axuser = {3'b0, ATTR[4:0], 4'b0} (same as AppIn0)
    uint32_t raw_attr = entry.attr.to_uint();
    axuser = (raw_attr & 0x1F) << 4;
    return true;
}

void TLBAppIn1::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) entries_[index] = entry;
}

TlbEntry TLBAppIn1::get_entry(uint8_t index) const {
    return (index < entries_.size()) ? entries_[index] : TlbEntry();
}

uint8_t TLBAppIn1::calculate_index(uint64_t addr) const {
    return static_cast<uint8_t>((addr >> 33) & 0x3F);
}

} // namespace pcie
} // namespace keraunos
