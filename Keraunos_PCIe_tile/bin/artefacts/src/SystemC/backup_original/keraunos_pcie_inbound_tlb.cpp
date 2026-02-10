#include "keraunos_pcie_inbound_tlb.h"
#include <cstring>
#include <iomanip>

namespace keraunos {
namespace pcie {

//=============================================================================
// TLBSysIn0 Implementation
//=============================================================================

TLBSysIn0::TLBSysIn0(sc_core::sc_module_name name)
    : sc_module(name)
    , config_adapter_("config_adapter", config_socket)
    , entries_(64)
    , tlb_memory_("tlb_memory", 64)  // 64 entries (size is in words, not bytes)
{
    // Initialize TLB entries
    for (auto& entry : entries_) {
        entry.valid = false;
        entry.addr = 0;
        entry.attr = 0;
    }
    
    // Bind SCML adapter to memory
    config_adapter_(tlb_memory_);
    
    // Bind TLM sockets for translation logic (passthrough)
    // simple_target_socket only requires b_transport; transport_dbg is optional
    inbound_socket.register_b_transport(this, &TLBSysIn0::b_transport);
    inbound_socket.register_transport_dbg(this, &TLBSysIn0::transport_dbg);
}

TLBSysIn0::~TLBSysIn0() {
}

uint8_t TLBSysIn0::calculate_index(uint64_t addr) const {
    // Index calculation: [51:14] -> [37:0] -> index [5:0]
    // 16KB page size: bits [13:0] are page offset
    return (addr >> 14) & 0x3F;
}

bool TLBSysIn0::lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser) {
    uint8_t index = calculate_index(iatu_addr);
    
    if (index >= entries_.size() || !entries_[index].valid) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    const TlbEntry& entry = entries_[index];
    
    // Translate address: {TLBSysIn0[index].ADDR[51:14], pa[13:0]}
    translated_addr = (entry.addr & 0xFFFFFFFFFC000ULL) | (iatu_addr & 0x3FFF);
    
    // Update AxUSER: {TLBSysIn0[index].ATTR[11:4], 2'b0, TLBSysIn0[index].ATTR[1:0]}
    // Note: ATTR[3:2] is always 00
    axuser = (entry.attr.range(11, 4).to_uint() << 4) | entry.attr.range(1, 0).to_uint();
    
    return true;
}

void TLBSysIn0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) {
        entries_[index] = entry;
    }
}

TlbEntry TLBSysIn0::get_entry(uint8_t index) const {
    if (index < entries_.size()) {
        return entries_[index];
    }
    return TlbEntry();  // Return invalid entry
}

void TLBSysIn0::update_axuser(uint32_t& axuser, const TlbEntry& entry) const {
    // Format: {ATTR[11:4], 2'b0, ATTR[1:0]}
    axuser = (entry.attr.range(11, 4).to_uint() << 4) | entry.attr.range(1, 0).to_uint();
}

void TLBSysIn0::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser = 0;
    
    // Perform TLB lookup
    if (!lookup(addr, translated_addr, axuser)) {
        // Invalid translation - return DECERR
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        
    }
    
    // Update transaction address
    trans.set_address(translated_addr);
    
    // Forward transaction to translated socket
    // Forward transaction directly - cannot copy tlm_generic_payload
    // simple_initiator_socket::operator->() provides access to TLM interface
    translated_socket->b_transport(trans, delay);
    
    // Copy response back
    
    
}

unsigned int TLBSysIn0::transport_dbg(tlm::tlm_generic_payload& trans) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser = 0;
    
    if (!lookup(addr, translated_addr, axuser)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return 0;
    }
    
    trans.set_address(translated_addr);
    return translated_socket->transport_dbg(trans);
}

// get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)

//=============================================================================
// TLBAppIn0 Implementation
//=============================================================================

TLBAppIn0::TLBAppIn0(sc_core::sc_module_name name, uint8_t instance_id)
    : sc_module(name)
    , config_adapter_("config_adapter", config_socket)
    , instance_id_(instance_id)
    , entries_(64)
    , tlb_memory_("tlb_memory", 64)  // 64 entries (size is in words, not bytes)
{
    // Initialize TLB entries
    for (auto& entry : entries_) {
        entry.valid = false;
        entry.addr = 0;
        entry.attr = 0;
    }
    
    // Bind SCML adapter to memory
    config_adapter_(tlb_memory_);
    
    // Bind TLM sockets for translation logic (passthrough)
    // simple_target_socket only requires b_transport; transport_dbg is optional
    inbound_socket.register_b_transport(this, &TLBAppIn0::b_transport);
    inbound_socket.register_transport_dbg(this, &TLBAppIn0::transport_dbg);
}

TLBAppIn0::~TLBAppIn0() {
}

uint8_t TLBAppIn0::calculate_index(uint64_t addr) const {
    // Index calculation: [51:24] -> [27:0] -> index [7:0]
    // 16MB page size: bits [23:0] are page offset
    // Note: Spec says 256 pages (0xFF), but we use 64 entries per instance
    return (addr >> 24) & 0x3F;
}

bool TLBAppIn0::lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser) {
    // Check port: iatu_addr[63:60] should be 0 for BAR0/1
    uint8_t port = (iatu_addr >> 60) & 0x1;
    if (port != 0) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    uint8_t index = calculate_index(iatu_addr);
    
    if (index >= entries_.size() || !entries_[index].valid) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    const TlbEntry& entry = entries_[index];
    
    // Translate address: {TLBAppIn0[index].ADDR[51:24], pa[23:0]}
    translated_addr = (entry.addr & 0xFFFFFF000000ULL) | (iatu_addr & 0xFFFFFF);
    
    // Update AxUSER: {3'b0, TLBAppIn0[index].ATTR[4:0], 4'b0}
    // [4] non-cacheable, [3:0] QoSID
    axuser = (entry.attr.range(4, 0).to_uint() << 4);
    
    return true;
}

void TLBAppIn0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) {
        entries_[index] = entry;
    }
}

TlbEntry TLBAppIn0::get_entry(uint8_t index) const {
    if (index < entries_.size()) {
        return entries_[index];
    }
    return TlbEntry();
}

void TLBAppIn0::update_axuser(uint32_t& axuser, const TlbEntry& entry) const {
    // Format: {3'b0, ATTR[4:0], 4'b0}
    axuser = (entry.attr.range(4, 0).to_uint() << 4);
}

void TLBAppIn0::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser = 0;
    
    if (!lookup(addr, translated_addr, axuser)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        
    }
    
    trans.set_address(translated_addr);
    
    // Forward transaction using simple_initiator_socket
    // Use operator->() to access the TLM interface
    translated_socket->b_transport(trans, delay);
    
}

unsigned int TLBAppIn0::transport_dbg(tlm::tlm_generic_payload& trans) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser = 0;
    
    if (!lookup(addr, translated_addr, axuser)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return 0;
    }
    
    trans.set_address(translated_addr);
    return translated_socket->transport_dbg(trans);
}

// get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)

//=============================================================================
// TLBAppIn1 Implementation
//=============================================================================

TLBAppIn1::TLBAppIn1(sc_core::sc_module_name name)
    : sc_module(name)
    , config_adapter_("config_adapter", config_socket)
    , entries_(64)
    , tlb_memory_("tlb_memory", 64)  // 64 entries (size is in words, not bytes)
{
    // Initialize TLB entries
    for (auto& entry : entries_) {
        entry.valid = false;
        entry.addr = 0;
        entry.attr = 0;
    }
    
    // Bind SCML adapter to memory
    config_adapter_(tlb_memory_);
    
    // Bind TLM sockets for translation logic (passthrough)
    // simple_target_socket only requires b_transport; transport_dbg is optional
    inbound_socket.register_b_transport(this, &TLBAppIn1::b_transport);
    inbound_socket.register_transport_dbg(this, &TLBAppIn1::transport_dbg);
}

TLBAppIn1::~TLBAppIn1() {
}

uint8_t TLBAppIn1::calculate_index(uint64_t addr) const {
    // Index calculation: [51:33] -> [18:0] -> index [5:0]
    // 8GB page size: bits [32:0] are page offset
    return (addr >> 33) & 0x3F;
}

bool TLBAppIn1::lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser) {
    // Check port: iatu_addr[63:60] should be 1 for BAR4/5
    uint8_t port = (iatu_addr >> 60) & 0x1;
    if (port != 1) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    uint8_t index = calculate_index(iatu_addr);
    
    if (index >= entries_.size() || !entries_[index].valid) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    const TlbEntry& entry = entries_[index];
    
    // Translate address: {TLBAppIn1[index].ADDR[51:33], pa[32:0]}
    translated_addr = (entry.addr & 0xFFFFFE00000000ULL) | (iatu_addr & 0x1FFFFFFFFULL);
    
    // Update AxUSER: {3'b0, TLBAppIn1[index].ATTR[4:0], 4'b0}
    // [4] non-cacheable, [3:0] QoSID
    axuser = (entry.attr.range(4, 0).to_uint() << 4);
    
    return true;
}

void TLBAppIn1::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) {
        entries_[index] = entry;
    }
}

TlbEntry TLBAppIn1::get_entry(uint8_t index) const {
    if (index < entries_.size()) {
        return entries_[index];
    }
    return TlbEntry();
}

void TLBAppIn1::update_axuser(uint32_t& axuser, const TlbEntry& entry) const {
    // Format: {3'b0, ATTR[4:0], 4'b0}
    axuser = (entry.attr.range(4, 0).to_uint() << 4);
}

void TLBAppIn1::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser = 0;
    
    if (!lookup(addr, translated_addr, axuser)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        
    }
    
    trans.set_address(translated_addr);
    
    // Forward transaction using simple_initiator_socket
    // Use operator->() to access the TLM interface
    translated_socket->b_transport(trans, delay);
    
}

unsigned int TLBAppIn1::transport_dbg(tlm::tlm_generic_payload& trans) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    uint32_t axuser = 0;
    
    if (!lookup(addr, translated_addr, axuser)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return 0;
    }
    
    trans.set_address(translated_addr);
    return translated_socket->transport_dbg(trans);
}

// get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)

} // namespace pcie
} // namespace keraunos

