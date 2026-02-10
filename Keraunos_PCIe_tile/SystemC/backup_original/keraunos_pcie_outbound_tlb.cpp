#include "keraunos_pcie_outbound_tlb.h"
#include "keraunos_pcie_common.h"
#include <cstring>
#include <iomanip>

namespace keraunos {
namespace pcie {

//=============================================================================
// TLBSysOut0 Implementation
//=============================================================================

TLBSysOut0::TLBSysOut0(sc_core::sc_module_name name)
    : sc_module(name)
    , config_adapter_("config_adapter", config_socket)
    , entries_(16)
    , tlb_memory_("tlb_memory", 16)  // 16 entries (size is in words, not bytes)
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
    outbound_socket.register_b_transport(this, &TLBSysOut0::b_transport);
    outbound_socket.register_transport_dbg(this, &TLBSysOut0::transport_dbg);
}

TLBSysOut0::~TLBSysOut0() {
}

uint8_t TLBSysOut0::calculate_index(uint64_t addr) const {
    // Index calculation: [63:16] -> [47:0] -> index [3:0]
    // 64KB page size: bits [15:0] are page offset
    return (addr >> 16) & 0xF;
}

bool TLBSysOut0::lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr) {
    uint8_t index = calculate_index(pa);
    
    if (index >= entries_.size() || !entries_[index].valid) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    const TlbEntry& entry = entries_[index];
    
    // Translate address: {TLBSysOut0[index].ADDR[63:16], pa[15:0]}
    translated_addr = (entry.addr & 0xFFFFFFFFFFFF0000ULL) | (pa & 0xFFFF);
    
    // Copy attributes
    attr = entry.attr;
    
    return true;
}

void TLBSysOut0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) {
        entries_[index] = entry;
    }
}

TlbEntry TLBSysOut0::get_entry(uint8_t index) const {
    if (index < entries_.size()) {
        return entries_[index];
    }
    return TlbEntry();
}

void TLBSysOut0::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Mask input address to 52 bits (outbound socket uses 64-bit but addresses are 52-bit)
    uint64_t addr = get_52bit_address(trans);
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (!lookup(addr, translated_addr, attr)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        
    }
    
    // Update transaction address
    trans.set_address(translated_addr);
    
    // TODO: Update AxUSER field with attr if needed
    
    // Forward transaction to translated socket
    // Forward transaction directly - cannot copy tlm_generic_payload
    // simple_initiator_socket::operator->() provides access to TLM interface
    translated_socket->b_transport(trans, delay);
    
    // Copy response back
    
    
}

unsigned int TLBSysOut0::transport_dbg(tlm::tlm_generic_payload& trans) {
    // Mask input address to 52 bits
    uint64_t addr = get_52bit_address(trans);
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (!lookup(addr, translated_addr, attr)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return 0;
    }
    
    trans.set_address(translated_addr);
    return translated_socket->transport_dbg(trans);
}

// get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)

//=============================================================================
// TLBAppOut0 Implementation
//=============================================================================

TLBAppOut0::TLBAppOut0(sc_core::sc_module_name name)
    : sc_module(name)
    , config_adapter_("config_adapter", config_socket)
    , entries_(16)
    , tlb_memory_("tlb_memory", 16)  // 16 entries (size is in words, not bytes)
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
    outbound_socket.register_b_transport(this, &TLBAppOut0::b_transport);
    outbound_socket.register_transport_dbg(this, &TLBAppOut0::transport_dbg);
}

TLBAppOut0::~TLBAppOut0() {
}

uint8_t TLBAppOut0::calculate_index(uint64_t addr) const {
    // Index calculation: [63:44] -> [19:0] -> index [3:0]
    // 16TB page size: bits [43:0] are page offset
    // Only used for addresses >= 256TB (pa >= (1 << 48))
    return (addr >> 44) & 0xF;
}

bool TLBAppOut0::lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr) {
    // Only process addresses >= 256TB
    if (pa < (1ULL << 48)) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    uint8_t index = calculate_index(pa);
    
    if (index >= entries_.size() || !entries_[index].valid) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    const TlbEntry& entry = entries_[index];
    
    // Translate address: {TLBAppOut0[index].ADDR[63:44], pa[43:0]}
    translated_addr = (entry.addr & 0xFFFFF00000000000ULL) | (pa & 0xFFFFFFFFFFFULL);
    
    // Copy attributes
    attr = entry.attr;
    
    return true;
}

void TLBAppOut0::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) {
        entries_[index] = entry;
    }
}

TlbEntry TLBAppOut0::get_entry(uint8_t index) const {
    if (index < entries_.size()) {
        return entries_[index];
    }
    return TlbEntry();
}

void TLBAppOut0::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (!lookup(addr, translated_addr, attr)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        
    }
    
    trans.set_address(translated_addr);
    
    // Forward transaction directly - cannot copy tlm_generic_payload
    // simple_initiator_socket::operator->() provides access to TLM interface
    translated_socket->b_transport(trans, delay);
    
    
    
}

unsigned int TLBAppOut0::transport_dbg(tlm::tlm_generic_payload& trans) {
    uint64_t addr = trans.get_address();
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (!lookup(addr, translated_addr, attr)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return 0;
    }
    
    trans.set_address(translated_addr);
    return translated_socket->transport_dbg(trans);
}

// get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)

//=============================================================================
// TLBAppOut1 Implementation
//=============================================================================

TLBAppOut1::TLBAppOut1(sc_core::sc_module_name name)
    : sc_module(name)
    , config_adapter_("config_adapter", config_socket)
    , entries_(16)
    , tlb_memory_("tlb_memory", 16)  // 16 entries (size is in words, not bytes)
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
    outbound_socket.register_b_transport(this, &TLBAppOut1::b_transport);
    outbound_socket.register_transport_dbg(this, &TLBAppOut1::transport_dbg);
}

TLBAppOut1::~TLBAppOut1() {
}

uint8_t TLBAppOut1::calculate_index(uint64_t addr) const {
    // Index calculation: [63:16] -> [47:0] -> index [3:0]
    // 64KB page size: bits [15:0] are page offset
    // Used for DBI access (addresses < 256TB)
    return (addr >> 16) & 0xF;
}

bool TLBAppOut1::lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr) {
    // Only process addresses < 256TB (DBI access)
    if (pa >= (1ULL << 48)) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    uint8_t index = calculate_index(pa);
    
    if (index >= entries_.size() || !entries_[index].valid) {
        translated_addr = INVALID_ADDRESS_DECERR;
        return false;
    }
    
    const TlbEntry& entry = entries_[index];
    
    // Translate address: {TLBAppOut1[index].ADDR[63:16], pa[15:0]}
    translated_addr = (entry.addr & 0xFFFFFFFFFFFF0000ULL) | (pa & 0xFFFF);
    
    // Copy attributes
    attr = entry.attr;
    
    return true;
}

void TLBAppOut1::configure_entry(uint8_t index, const TlbEntry& entry) {
    if (index < entries_.size()) {
        entries_[index] = entry;
    }
}

TlbEntry TLBAppOut1::get_entry(uint8_t index) const {
    if (index < entries_.size()) {
        return entries_[index];
    }
    return TlbEntry();
}

void TLBAppOut1::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Mask input address to 52 bits (outbound socket uses 64-bit but addresses are 52-bit)
    uint64_t addr = get_52bit_address(trans);
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (!lookup(addr, translated_addr, attr)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        
    }
    
    trans.set_address(translated_addr);
    
    // Forward transaction directly - cannot copy tlm_generic_payload
    // simple_initiator_socket::operator->() provides access to TLM interface
    translated_socket->b_transport(trans, delay);
    
    
    
}

unsigned int TLBAppOut1::transport_dbg(tlm::tlm_generic_payload& trans) {
    // Mask input address to 52 bits
    uint64_t addr = get_52bit_address(trans);
    uint64_t translated_addr;
    sc_dt::sc_bv<256> attr;
    
    if (!lookup(addr, translated_addr, attr)) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return 0;
    }
    
    trans.set_address(translated_addr);
    return translated_socket->transport_dbg(trans);
}

// get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)

} // namespace pcie
} // namespace keraunos

