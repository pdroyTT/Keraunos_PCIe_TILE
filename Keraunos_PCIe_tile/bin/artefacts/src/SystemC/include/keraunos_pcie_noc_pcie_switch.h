#ifndef KERAUNOS_PCIE_NOC_PCIE_SWITCH_H
#define KERAUNOS_PCIE_NOC_PCIE_SWITCH_H

// REFACTORED: C++ class with function callbacks

#include "keraunos_pcie_common.h"
#include "scml2/tagged_message_macros.h"
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
    
    /**
     * @brief Set isolation request
     * @param val Isolation state (true = isolated)
     */
    void set_isolate_req(const bool val) noexcept { isolate_req_ = val; }
    
    /**
     * @brief Set PCIe outbound enable
     * @param val Outbound enable state
     */
    void set_pcie_outbound_app_enable(const bool val) noexcept { pcie_outbound_enable_ = val; }
    
    /**
     * @brief Set PCIe inbound enable
     * @param val Inbound enable state
     */
    void set_pcie_inbound_app_enable(const bool val) noexcept { pcie_inbound_enable_ = val; }
    
    /**
     * @brief Set system ready status
     * @param val System ready state
     */
    void set_system_ready(const bool val) noexcept { system_ready_ = val; }
    
    /**
     * @brief Set Bus Master Enable state (from PCIe controller Command Register)
     * @param val BME state per Section 2.5.8.1, Table 33
     */
    void set_bus_master_enable(const bool val) noexcept { bus_master_enable_ = val; }
    
    /**
     * @brief Set controller mode (Endpoint vs Root Port)
     * @param val true = Endpoint mode, false = Root Port mode
     */
    void set_controller_is_ep(const bool val) noexcept { controller_is_ep_ = val; }
    
    /**
     * @brief Get Bus Master Enable state
     * @return Current BME state
     */
    [[nodiscard]] bool get_bus_master_enable() const noexcept { return bus_master_enable_; }
    
    /**
     * @brief Get controller mode
     * @return true = Endpoint, false = Root Port
     */
    [[nodiscard]] bool get_controller_is_ep() const noexcept { return controller_is_ep_; }
    
    /**
     * @brief Get status register value
     * @return Status register content
     */
    [[nodiscard]] uint32_t get_status_reg_value() const noexcept { return system_ready_ ? 1 : 0; }
    
    /**
     * @brief Set parent module pointer for FastTrack logging
     * @param parent_module Pointer to parent sc_module for logging context
     */
    void set_parent_module(sc_core::sc_module* parent_module) noexcept { parent_module_ = parent_module; }
    
    /**
     * @brief Enable or disable debug logging
     * @param enable true to enable FastTrack debug logging
     */
    void set_debug_logging_enabled(bool enable) noexcept { debug_logging_enabled_ = enable; }
    
private:
    bool isolate_req_, pcie_outbound_enable_, pcie_inbound_enable_, system_ready_;
    bool bus_master_enable_;
    bool controller_is_ep_;
    bool debug_logging_enabled_;
    sc_core::sc_module* parent_module_;
    TransportCallback tlb_app_inbound0_, tlb_app_inbound1_, tlb_sys_inbound_;
    TransportCallback tlb_app_out0_, tlb_app_out1_, tlb_sys_out0_;
    TransportCallback noc_io_, smn_io_, pcie_controller_, msi_relay_, config_reg_;
    std::map<uint64_t, OutstandingRequest> outstanding_requests_;
    uint64_t next_request_id_;
    
    /**
     * @brief Decode address to determine routing destination
     * @param addr Transaction address with routing bits [63:60]
     * @param is_read true for read, false for write
     * @return Route destination enum
     */
    NocPcieRoute route_address(uint64_t addr, bool is_read) const;
    
    /**
     * @brief Check if address is status register access
     * @param addr Transaction address
     * @param is_read true for read, false for write
     * @return true if status register access
     */
    bool is_status_register_access(uint64_t addr, bool is_read) const;
    
    /**
     * @brief Check if transaction is exempt from BME qualification
     * @param axuser AxUSER sideband with TLP type (Table 34)
     * @return true if exempt (CfgRd/Wr, Msg/MsgD, DBI)
     */
    bool is_bme_exempt(const sc_dt::sc_bv<256>& axuser) const;
};

} // namespace pcie
} // namespace keraunos

#endif
