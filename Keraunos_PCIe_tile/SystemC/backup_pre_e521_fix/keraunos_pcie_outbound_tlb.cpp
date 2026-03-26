#include "keraunos_pcie_outbound_tlb.h"
#include <cstring>
#include <fstream>

namespace keraunos {
namespace pcie {

// TLBSysOut0 - already implemented in inbound file as they share similar logic
// Just copy implementation here for completeness
TLBSysOut0::TLBSysOut0() : entries_(16), tlb_memory_("tlb_sys_out0_memory", 4096) {
    // Initialize entry 0 as valid
    entries_[0].valid = true;
    entries_[0].addr = 0x4000000000 >> 12;
    entries_[0].attr = 0;
}

void TLBSysOut0::process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
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

void TLBSysOut0::process_outbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t pa = trans.get_address();
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (lookup(pa, translated_addr, attr)) {
        trans.set_address(translated_addr);
        if (translated_output_) {
            translated_output_(trans, delay, attr);  // Pass AxUSER for BME qualification
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

bool TLBSysOut0::lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr) {
    uint8_t index = calculate_index(pa);
    if (index >= entries_.size()) return false;
    const TlbEntry& entry = entries_[index];
    if (!entry.valid) return false;
    // Spec: translated = {ADDR[63:16], pa[15:0]}  (16-bit page size / 64KB)
    constexpr uint64_t page_mask = (1ULL << 16) - 1;
    translated_addr = ((entry.addr << 12) & ~page_mask) | (pa & page_mask);
    attr = entry.attr;
    return true;
}

void TLBSysOut0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) entries_[index] = entry;
}

TlbEntry TLBSysOut0::get_entry(uint8_t index) const {
    return (index < entries_.size()) ? entries_[index] : TlbEntry();
}

uint8_t TLBSysOut0::calculate_index(uint64_t addr) const {
    return static_cast<uint8_t>((addr >> 16) & 0xF);
}

// TLBAppOut0
TLBAppOut0::TLBAppOut0() : entries_(16), tlb_memory_("tlb_app_out0_memory", 4096) {
    // Initialize entry 0 as valid
    // Base must be 16TB-aligned (bits[63:44] populated) for 44-bit page TLB
    entries_[0].valid = true;
    entries_[0].addr = 0xA00000000000ULL >> 12;
    entries_[0].attr = 0;
}

void TLBAppOut0::process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
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

void TLBAppOut0::process_outbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t pa = trans.get_address();
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (lookup(pa, translated_addr, attr)) {
        trans.set_address(translated_addr);
        if (translated_output_) {
            translated_output_(trans, delay, attr);  // Pass AxUSER for BME qualification
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

bool TLBAppOut0::lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr) {
    uint8_t index = calculate_index(pa);
    if (index >= entries_.size()) return false;
    const TlbEntry& entry = entries_[index];
    if (!entry.valid) return false;
    // Spec: translated = {ADDR[63:44], pa[43:0]}  (44-bit page size / 16TB)
    constexpr uint64_t page_mask = (1ULL << 44) - 1;
    translated_addr = ((entry.addr << 12) & ~page_mask) | (pa & page_mask);
    attr = entry.attr;
    return true;
}

void TLBAppOut0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) entries_[index] = entry;
}

TlbEntry TLBAppOut0::get_entry(uint8_t index) const {
    return (index < entries_.size()) ? entries_[index] : TlbEntry();
}

uint8_t TLBAppOut0::calculate_index(uint64_t addr) const {
    return static_cast<uint8_t>((addr >> 44) & 0xF);
}

// TLBAppOut1
TLBAppOut1::TLBAppOut1() : entries_(16), tlb_memory_("tlb_app_out1_memory", 4096) {
    // Initialize entry 0 as valid
    entries_[0].valid = true;
    entries_[0].addr = 0x9000000000 >> 12;
    entries_[0].attr = 0;
}

void TLBAppOut1::process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
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

void TLBAppOut1::process_outbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t pa = trans.get_address();
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (lookup(pa, translated_addr, attr)) {
        trans.set_address(translated_addr);
        if (translated_output_) {
            translated_output_(trans, delay, attr);  // Pass AxUSER for BME qualification
        } else {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

bool TLBAppOut1::lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr) {
    uint8_t index = calculate_index(pa);
    if (index >= entries_.size()) return false;
    const TlbEntry& entry = entries_[index];
    if (!entry.valid) return false;
    // Spec: translated = {ADDR[63:16], pa[15:0]}  (16-bit page size / 64KB)
    constexpr uint64_t page_mask = (1ULL << 16) - 1;
    translated_addr = ((entry.addr << 12) & ~page_mask) | (pa & page_mask);
    attr = entry.attr;
    return true;
}

void TLBAppOut1::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) entries_[index] = entry;
}

TlbEntry TLBAppOut1::get_entry(uint8_t index) const {
    return (index < entries_.size()) ? entries_[index] : TlbEntry();
}

uint8_t TLBAppOut1::calculate_index(uint64_t addr) const {
    return static_cast<uint8_t>((addr >> 16) & 0xF);
}

} // namespace pcie
} // namespace keraunos
