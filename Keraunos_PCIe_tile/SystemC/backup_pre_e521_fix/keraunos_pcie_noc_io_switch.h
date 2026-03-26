#ifndef KERAUNOS_PCIE_NOC_IO_SWITCH_H
#define KERAUNOS_PCIE_NOC_IO_SWITCH_H

// REFACTORED: C++ class with function callbacks

#include "keraunos_pcie_common.h"
#include <systemc>
#include <tlm>
#include <functional>
#include <map>
#include <cstdint>

namespace keraunos {
namespace pcie {

class NocIoSwitch {
public:
    NocIoSwitch();
    ~NocIoSwitch() = default;
    
    // Function interfaces
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    
    // Inbound from NOC-N or NOC-PCIE switch
    void route_from_noc(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Inbound from TLB (after translation, going to external NOC-N)
    void route_from_tlb(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Set callbacks for routing destinations
    void set_noc_n_output(TransportCallback cb) { noc_n_output_ = cb; }
    void set_tlb_app_output(TransportCallback cb) { tlb_app_output_ = cb; }
    void set_msi_relay_output(TransportCallback cb) { msi_relay_output_ = cb; }
    
    // Control (noexcept for performance)
    void set_isolate_req(const bool val) noexcept { isolate_req_ = val; }
    [[nodiscard]] bool get_timeout_read() const noexcept { return timeout_read_; }
    [[nodiscard]] bool get_timeout_write() const noexcept { return timeout_write_; }
    
private:
    bool isolate_req_, timeout_read_, timeout_write_;
    TransportCallback noc_n_output_, tlb_app_output_, msi_relay_output_;
    std::map<uint64_t, OutstandingRequest> outstanding_requests_;
    uint64_t next_request_id_;
    
    bool route_to_noc_n(uint64_t addr) const;
};

} // namespace pcie
} // namespace keraunos

#endif
