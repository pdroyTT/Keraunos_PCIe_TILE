#ifndef KERAUNOS_PCIE_INBOUND_TLB_H
#define KERAUNOS_PCIE_INBOUND_TLB_H

#include "keraunos_pcie_tlb_common.h"
#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <sc_dt.h>
#include <vector>

namespace keraunos {
namespace pcie {

// Forward declaration
class InboundTlbBase;

/**
 * Inbound TLB for System Management (TLBSysIn0)
 * - 64 entries, 16KB page size
 * - Address range: [51:14] for indexing
 * - AxUSER: [11:0] (12 bits)
 */
class TLBSysIn0 : public sc_core::sc_module {
public:
    // TLM target socket for configuration (APB)
    tlm::tlm_base_target_socket<32> config_socket;
    
    // TLM target socket for inbound traffic (AXI4)
    tlm_utils::simple_target_socket<TLBSysIn0, 64> inbound_socket;
    
    // TLM initiator socket for translated traffic (AXI4)
    scml2::initiator_socket<64> translated_socket;
    
    // Control signals
    sc_core::sc_in<bool> system_ready;  // System ready bit
    
    SC_HAS_PROCESS(TLBSysIn0);
    
    TLBSysIn0(sc_core::sc_module_name name);
    virtual ~TLBSysIn0();
    
    // TLB lookup function
    // Returns translated address and attributes, or INVALID_ADDRESS_DECERR if invalid
    bool lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser);
    
    // Configuration access
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
protected:
    // SCML2 port adapter for configuration socket
    scml2::tlm2_gp_target_adapter<32> config_adapter_;
    
    // TLB entries (64 entries)
    std::vector<TlbEntry> entries_;
    
    // SCML2 memory for TLB configuration space
    scml2::memory<unsigned long long> tlb_memory_;
    
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    // get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)
    
    // Helper methods
    uint8_t calculate_index(uint64_t addr) const;
    void update_axuser(uint32_t& axuser, const TlbEntry& entry) const;
};

/**
 * Inbound TLB for Application BAR0/1 (TLBAppIn0)
 * - 64 entries per instance, 16MB page size
 * - Multiple instances: TLBAppIn0-0, TLBAppIn0-1, TLBAppIn0-2, TLBAppIn0-3
 * - Address range: [51:24] for indexing
 * - AxUSER: [4:0] (5 bits: [4] non-cacheable, [3:0] QoSID)
 */
class TLBAppIn0 : public sc_core::sc_module {
public:
    // TLM target socket for configuration (APB)
    tlm::tlm_base_target_socket<32> config_socket;
    
    // TLM target socket for inbound traffic (AXI4)
    tlm_utils::simple_target_socket<TLBAppIn0, 64> inbound_socket;
    
    // TLM initiator socket for translated traffic (AXI4)
    scml2::initiator_socket<64> translated_socket;
    
    // Instance ID (0-3)
    const uint8_t instance_id_;
    
    SC_HAS_PROCESS(TLBAppIn0);
    
    TLBAppIn0(sc_core::sc_module_name name, uint8_t instance_id = 0);
    virtual ~TLBAppIn0();
    
    // TLB lookup function
    // iatu_addr[63:60] = 0 for BAR0/1
    bool lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser);
    
    // Configuration access
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
protected:
    // SCML2 port adapter for configuration socket
    scml2::tlm2_gp_target_adapter<32> config_adapter_;
    
    // TLB entries (64 entries)
    std::vector<TlbEntry> entries_;
    
    // SCML2 memory for TLB configuration space
    scml2::memory<unsigned long long> tlb_memory_;
    
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    // get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)
    
    // Helper methods
    uint8_t calculate_index(uint64_t addr) const;
    void update_axuser(uint32_t& axuser, const TlbEntry& entry) const;
};

/**
 * Inbound TLB for Application BAR4/5 (TLBAppIn1)
 * - 64 entries, 8GB page size
 * - Address range: [51:33] for indexing
 * - AxUSER: [4:0] (5 bits: [4] non-cacheable, [3:0] QoSID)
 */
class TLBAppIn1 : public sc_core::sc_module {
public:
    // TLM target socket for configuration (APB)
    tlm::tlm_base_target_socket<32> config_socket;
    
    // TLM target socket for inbound traffic (AXI4)
    tlm_utils::simple_target_socket<TLBAppIn1, 64> inbound_socket;
    
    // TLM initiator socket for translated traffic (AXI4)
    scml2::initiator_socket<64> translated_socket;
    
    SC_HAS_PROCESS(TLBAppIn1);
    
    TLBAppIn1(sc_core::sc_module_name name);
    virtual ~TLBAppIn1();
    
    // TLB lookup function
    // iatu_addr[63:60] = 1 for BAR4/5
    bool lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser);
    
    // Configuration access
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
protected:
    // SCML2 port adapter for configuration socket
    scml2::tlm2_gp_target_adapter<32> config_adapter_;
    
    // TLB entries (64 entries)
    std::vector<TlbEntry> entries_;
    
    // SCML2 memory for TLB configuration space
    scml2::memory<unsigned long long> tlb_memory_;
    
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    // get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)
    
    // Helper methods
    uint8_t calculate_index(uint64_t addr) const;
    void update_axuser(uint32_t& axuser, const TlbEntry& entry) const;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_INBOUND_TLB_H

