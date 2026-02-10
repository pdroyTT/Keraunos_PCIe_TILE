#ifndef KERAUNOS_PCIE_OUTBOUND_TLB_H
#define KERAUNOS_PCIE_OUTBOUND_TLB_H

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

/**
 * Outbound TLB for System Management (TLBSysOut0)
 * - 16 entries, 64KB page size
 * - Address range: [63:16] for indexing
 * - Used for DBI access
 */
class TLBSysOut0 : public sc_core::sc_module {
public:
    // TLM target socket for configuration (APB)
    tlm::tlm_base_target_socket<32> config_socket;
    
    // TLM target socket for outbound traffic (AXI4)
    // Using 64-bit socket, addresses masked to 52 bits
    tlm_utils::simple_target_socket<TLBSysOut0, 64> outbound_socket;
    
    // TLM initiator socket for translated traffic (AXI4)
    scml2::initiator_socket<64> translated_socket;
    
    SC_HAS_PROCESS(TLBSysOut0);
    
    TLBSysOut0(sc_core::sc_module_name name);
    virtual ~TLBSysOut0();
    
    // TLB lookup function
    // Returns translated address and attributes, or INVALID_ADDRESS_DECERR if invalid
    bool lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr);
    
    // Configuration access
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
protected:
    // SCML2 port adapter for configuration socket
    scml2::tlm2_gp_target_adapter<32> config_adapter_;
    
    // TLB entries (16 entries)
    std::vector<TlbEntry> entries_;
    
    // SCML2 memory for TLB configuration space
    scml2::memory<unsigned long long> tlb_memory_;
    
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    // get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)
    
    // Helper methods
    uint8_t calculate_index(uint64_t addr) const;
};

/**
 * Outbound TLB for Application from Application Network (TLBAppOut0)
 * - 16 entries, 16TB page size
 * - Address range: [63:44] for indexing
 * - Used for regular memory accesses above 256TB
 */
class TLBAppOut0 : public sc_core::sc_module {
public:
    // TLM target socket for configuration (APB)
    tlm::tlm_base_target_socket<32> config_socket;
    
    // TLM target socket for outbound traffic (AXI4)
    // Using 64-bit socket, addresses masked to 52 bits
    tlm_utils::simple_target_socket<TLBAppOut0, 64> outbound_socket;
    
    // TLM initiator socket for translated traffic (AXI4)
    scml2::initiator_socket<64> translated_socket;
    
    SC_HAS_PROCESS(TLBAppOut0);
    
    TLBAppOut0(sc_core::sc_module_name name);
    virtual ~TLBAppOut0();
    
    // TLB lookup function
    // For addresses >= 256TB (pa >= (1 << 48))
    bool lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr);
    
    // Configuration access
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
protected:
    // SCML2 port adapter for configuration socket
    scml2::tlm2_gp_target_adapter<32> config_adapter_;
    
    // TLB entries (16 entries)
    std::vector<TlbEntry> entries_;
    
    // SCML2 memory for TLB configuration space
    scml2::memory<unsigned long long> tlb_memory_;
    
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    // get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)
    
    // Helper methods
    uint8_t calculate_index(uint64_t addr) const;
};

/**
 * Outbound TLB for Application from System Management Network (TLBAppOut1)
 * - 16 entries, 64KB page size
 * - Address range: [63:16] for indexing
 * - Used for DBI access
 */
class TLBAppOut1 : public sc_core::sc_module {
public:
    // TLM target socket for configuration (APB)
    tlm::tlm_base_target_socket<32> config_socket;
    
    // TLM target socket for outbound traffic (AXI4)
    // Using 64-bit socket, addresses masked to 52 bits
    tlm_utils::simple_target_socket<TLBAppOut1, 64> outbound_socket;
    
    // TLM initiator socket for translated traffic (AXI4)
    scml2::initiator_socket<64> translated_socket;
    
    SC_HAS_PROCESS(TLBAppOut1);
    
    TLBAppOut1(sc_core::sc_module_name name);
    virtual ~TLBAppOut1();
    
    // TLB lookup function
    // For addresses < 256TB (DBI access)
    bool lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr);
    
    // Configuration access
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
protected:
    // SCML2 port adapter for configuration socket
    scml2::tlm2_gp_target_adapter<32> config_adapter_;
    
    // TLB entries (16 entries)
    std::vector<TlbEntry> entries_;
    
    // SCML2 memory for TLB configuration space
    scml2::memory<unsigned long long> tlb_memory_;
    
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    // get_direct_mem_ptr not needed - simple_target_socket provides default (returns false)
    
    // Helper methods
    uint8_t calculate_index(uint64_t addr) const;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_OUTBOUND_TLB_H

