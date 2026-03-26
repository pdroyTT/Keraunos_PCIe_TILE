#ifndef KERAUNOS_PCIE_OUTBOUND_TLB_H
#define KERAUNOS_PCIE_OUTBOUND_TLB_H

// REFACTORED: C++ classes with function callbacks and SCML2 memory

#include "keraunos_pcie_tlb_common.h"
#include <scml2.h>
#include <scml2/memory.h>
#include <systemc>
#include <tlm>
#include <functional>
#include <vector>

namespace keraunos {
namespace pcie {

class TLBSysOut0 {
public:
    TLBSysOut0();
    ~TLBSysOut0() = default;
    
    // 3-arg callback: carries AxUSER (TLB ATTR) for downstream BME qualification
    using TransportWithAttrCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&, const sc_dt::sc_bv<256>&)>;
    
    void process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_outbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_translated_output(TransportWithAttrCallback cb) { translated_output_ = cb; }
    bool lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr);
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
private:
    std::vector<TlbEntry> entries_;
    TransportWithAttrCallback translated_output_;
    scml2::memory<uint8_t> tlb_memory_;  // SCML2 memory
    uint8_t calculate_index(uint64_t addr) const;
};

class TLBAppOut0 {
public:
    TLBAppOut0();
    ~TLBAppOut0() = default;
    
    // 3-arg callback: carries AxUSER (TLB ATTR) for downstream BME qualification
    using TransportWithAttrCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&, const sc_dt::sc_bv<256>&)>;
    
    void process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_outbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_translated_output(TransportWithAttrCallback cb) { translated_output_ = cb; }
    bool lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr);
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
private:
    std::vector<TlbEntry> entries_;
    TransportWithAttrCallback translated_output_;
    scml2::memory<uint8_t> tlb_memory_;  // SCML2 memory
    uint8_t calculate_index(uint64_t addr) const;
};

class TLBAppOut1 {
public:
    TLBAppOut1();
    ~TLBAppOut1() = default;
    
    // 3-arg callback: carries AxUSER (TLB ATTR) for downstream BME qualification
    using TransportWithAttrCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&, const sc_dt::sc_bv<256>&)>;
    
    void process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_outbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_translated_output(TransportWithAttrCallback cb) { translated_output_ = cb; }
    bool lookup(uint64_t pa, uint64_t& translated_addr, sc_dt::sc_bv<256>& attr);
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
private:
    std::vector<TlbEntry> entries_;
    TransportWithAttrCallback translated_output_;
    scml2::memory<uint8_t> tlb_memory_;  // SCML2 memory
    uint8_t calculate_index(uint64_t addr) const;
};

} // namespace pcie
} // namespace keraunos

#endif
