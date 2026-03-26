#ifndef KERAUNOS_PCIE_INBOUND_TLB_H
#define KERAUNOS_PCIE_INBOUND_TLB_H

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

class TLBSysIn0 {
public:
    TLBSysIn0();
    ~TLBSysIn0() = default;
    
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    
    void process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_inbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_translated_output(TransportCallback cb) { translated_output_ = cb; }
    void set_system_ready(bool val) { system_ready_ = val; }
    bool lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser);
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
private:
    std::vector<TlbEntry> entries_;
    bool system_ready_;
    TransportCallback translated_output_;
    scml2::memory<uint8_t> tlb_memory_;  // SCML2 memory for config
    uint8_t calculate_index(uint64_t addr) const;
};

class TLBAppIn0 {
public:
    explicit TLBAppIn0(uint8_t instance_id = 0);
    ~TLBAppIn0() = default;
    
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    
    void process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_inbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_translated_output(TransportCallback cb) { translated_output_ = cb; }
    bool lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser);
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
private:
    const uint8_t instance_id_;
    std::vector<TlbEntry> entries_;
    TransportCallback translated_output_;
    scml2::memory<uint8_t> tlb_memory_;  // SCML2 memory
    uint8_t calculate_index(uint64_t addr) const;
};

class TLBAppIn1 {
public:
    TLBAppIn1();
    ~TLBAppIn1() = default;
    
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    
    void process_config_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_inbound_traffic(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_translated_output(TransportCallback cb) { translated_output_ = cb; }
    bool lookup(uint64_t iatu_addr, uint64_t& translated_addr, uint32_t& axuser);
    void configure_entry(uint8_t index, const TlbEntry& entry);
    TlbEntry get_entry(uint8_t index) const;
    
private:
    std::vector<TlbEntry> entries_;
    TransportCallback translated_output_;
    scml2::memory<uint8_t> tlb_memory_;  // SCML2 memory
    uint8_t calculate_index(uint64_t addr) const;
};

} // namespace pcie
} // namespace keraunos

#endif
