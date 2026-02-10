#ifndef KERAUNOS_PCIE_SMN_IO_SWITCH_H
#define KERAUNOS_PCIE_SMN_IO_SWITCH_H

// REFACTORED: C++ class with function callbacks

#include "keraunos_pcie_common.h"
#include <systemc>
#include <tlm>
#include <functional>
#include <map>

namespace keraunos {
namespace pcie {

class SmnIoSwitch {
public:
    SmnIoSwitch();
    ~SmnIoSwitch() = default;
    
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    
    // Inbound from SMN-N or NOC-PCIE switch
    void route_from_smn(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Set callbacks for routing
    void set_smn_n_output(TransportCallback cb) { smn_n_output_ = cb; }
    void set_tlb_sys_inbound_output(TransportCallback cb) { tlb_sys_inbound_ = cb; }
    void set_tlb_sys_outbound_output(TransportCallback cb) { tlb_sys_outbound_ = cb; }
    void set_config_reg_output(TransportCallback cb) { config_reg_ = cb; }
    void set_msi_relay_cfg_output(TransportCallback cb) { msi_relay_cfg_ = cb; }
    void set_sii_config_output(TransportCallback cb) { sii_config_ = cb; }
    void set_serdes_apb_output(TransportCallback cb) { serdes_apb_ = cb; }
    void set_serdes_ahb_output(TransportCallback cb) { serdes_ahb_ = cb; }
    void set_tlb_sys_in0_cfg_output(TransportCallback cb) { tlb_sys_in0_cfg_ = cb; }
    void set_tlb_app_in0_cfg_output(int idx, TransportCallback cb) { if(idx<4) tlb_app_in0_cfg_[idx] = cb; }
    void set_tlb_app_in1_cfg_output(TransportCallback cb) { tlb_app_in1_cfg_ = cb; }
    void set_tlb_sys_out0_cfg_output(TransportCallback cb) { tlb_sys_out0_cfg_ = cb; }
    void set_tlb_app_out0_cfg_output(TransportCallback cb) { tlb_app_out0_cfg_ = cb; }
    void set_tlb_app_out1_cfg_output(TransportCallback cb) { tlb_app_out1_cfg_ = cb; }
    
    // Route from NOC-IO switch
    void route_from_noc_io(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_msi_relay_data_output(TransportCallback cb) { msi_relay_data_ = cb; }
    
    void set_isolate_req(const bool val) noexcept { isolate_req_ = val; }
    void set_isolation(bool isolate);
    [[nodiscard]] bool get_timeout() const noexcept { return timeout_; }
    bool get_timeout_status() const;
    
private:
    bool isolate_req_, timeout_;
    TransportCallback smn_n_output_, tlb_sys_inbound_, tlb_sys_outbound_;
    TransportCallback config_reg_, msi_relay_cfg_, msi_relay_data_, sii_config_, serdes_apb_, serdes_ahb_;
    TransportCallback tlb_sys_in0_cfg_, tlb_app_in0_cfg_[4], tlb_app_in1_cfg_;
    TransportCallback tlb_sys_out0_cfg_, tlb_app_out0_cfg_, tlb_app_out1_cfg_;
    std::map<uint64_t, OutstandingRequest> outstanding_requests_;
    uint64_t next_request_id_;
};

} // namespace pcie
} // namespace keraunos

#endif
