#ifndef KERAUNOS_PCIE_NOC_PCIE_SWITCH_H
#define KERAUNOS_PCIE_NOC_PCIE_SWITCH_H

// REFACTORED: C++ class with function callbacks

#include "keraunos_pcie_common.h"
#include <systemc>
#include <tlm>
#include <functional>
#include <map>

namespace keraunos {
namespace pcie {

class NocPcieSwitch {
public:
    NocPcieSwitch();
    ~NocPcieSwitch() = default;
    
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    
    // Inbound from PCIe Controller
    void route_from_pcie(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Outbound from TLBs or switches back to PCIe
    void route_to_pcie(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    // Overload with AxUSER for BME qualification (Table 33, Section 2.5.8.1)
    void route_to_pcie(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay,
                       const sc_dt::sc_bv<256>& axuser);
    
    // Set callbacks for routing
    void set_tlb_app_inbound0_output(TransportCallback cb) { tlb_app_inbound0_ = cb; }
    void set_tlb_app_inbound1_output(TransportCallback cb) { tlb_app_inbound1_ = cb; }
    void set_tlb_sys_inbound_output(TransportCallback cb) { tlb_sys_inbound_ = cb; }
    void set_tlb_app_out0_output(TransportCallback cb) { tlb_app_out0_ = cb; }
    void set_tlb_app_out1_output(TransportCallback cb) { tlb_app_out1_ = cb; }
    void set_tlb_sys_out0_output(TransportCallback cb) { tlb_sys_out0_ = cb; }
    void set_noc_io_output(TransportCallback cb) { noc_io_ = cb; }
    void set_smn_io_output(TransportCallback cb) { smn_io_ = cb; }
    void set_pcie_controller_output(TransportCallback cb) { pcie_controller_ = cb; }
    void set_msi_relay_output(TransportCallback cb) { msi_relay_ = cb; }
    void set_config_reg_output(TransportCallback cb) { config_reg_ = cb; }
    
    // Control
    void set_isolate_req(const bool val) noexcept { isolate_req_ = val; }
    void set_pcie_outbound_app_enable(const bool val) noexcept { pcie_outbound_enable_ = val; }
    void set_pcie_inbound_app_enable(const bool val) noexcept { pcie_inbound_enable_ = val; }
    void set_system_ready(const bool val) noexcept { system_ready_ = val; }
    // BME / controller mode (Section 2.5.8.1, Table 33)
    void set_bus_master_enable(const bool val) noexcept { bus_master_enable_ = val; }
    void set_controller_is_ep(const bool val) noexcept { controller_is_ep_ = val; }
    [[nodiscard]] bool get_bus_master_enable() const noexcept { return bus_master_enable_; }
    [[nodiscard]] bool get_controller_is_ep() const noexcept { return controller_is_ep_; }
    [[nodiscard]] uint32_t get_status_reg_value() const noexcept { return system_ready_ ? 1 : 0; }
    
private:
    bool isolate_req_, pcie_outbound_enable_, pcie_inbound_enable_, system_ready_;
    bool bus_master_enable_;    // PCIe Bus Master Enable (from controller Command Register)
    bool controller_is_ep_;     // true = EP mode, false = RP mode (from SII device_type)
    TransportCallback tlb_app_inbound0_, tlb_app_inbound1_, tlb_sys_inbound_;
    TransportCallback tlb_app_out0_, tlb_app_out1_, tlb_sys_out0_;
    TransportCallback noc_io_, smn_io_, pcie_controller_, msi_relay_, config_reg_;
    std::map<uint64_t, OutstandingRequest> outstanding_requests_;
    uint64_t next_request_id_;
    
    NocPcieRoute route_address(uint64_t addr, bool is_read) const;
    bool is_status_register_access(uint64_t addr, bool is_read) const;
    // BME exemption check per Table 34: CfgRd/Wr, Msg/MsgD, and DBI are not affected by BME
    bool is_bme_exempt(const sc_dt::sc_bv<256>& axuser) const;
};

} // namespace pcie
} // namespace keraunos

#endif
