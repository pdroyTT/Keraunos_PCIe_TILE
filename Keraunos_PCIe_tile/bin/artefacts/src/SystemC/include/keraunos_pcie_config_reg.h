#ifndef KERAUNOS_PCIE_CONFIG_REG_H
#define KERAUNOS_PCIE_CONFIG_REG_H

// REFACTORED: C++ class with callback for value change notification

#include <scml2.h>
#include <scml2/memory.h>
#include "scml2/tagged_message_macros.h"
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <functional>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * @brief Configuration Register Block with callback for changes
 * 
 * Implements the configuration register space for the PCIe tile,
 * using SCML2 memory objects for persistent storage. Provides
 * callbacks for configuration changes and FastTrack logging.
 */
class ConfigRegBlock {
public:
    ConfigRegBlock();
    ~ConfigRegBlock() = default;
    
    /**
     * @brief Process APB configuration access (read or write)
     * @param trans TLM2.0 generic payload
     * @param delay Temporal decoupling delay (loosely-timed)
     */
    void process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    /**
     * @brief Get system ready status
     * @return true if system is ready for operation
     */
    [[nodiscard]] bool get_system_ready() const noexcept { return system_ready_; }
    
    /**
     * @brief Get PCIe outbound application enable status
     * @return true if outbound traffic is enabled
     */
    [[nodiscard]] bool get_pcie_outbound_app_enable() const noexcept { return pcie_outbound_app_enable_; }
    
    /**
     * @brief Get PCIe inbound application enable status
     * @return true if inbound traffic is enabled
     */
    [[nodiscard]] bool get_pcie_inbound_app_enable() const noexcept { return pcie_inbound_app_enable_; }
    
    /**
     * @brief Set isolation request state
     * @param isolate true to isolate (disables all traffic)
     */
    void set_isolate_req(const bool isolate) noexcept;
    
    /**
     * @brief Set debug logging enable state
     * @param enable true to enable FastTrack debug logging
     */
    void set_debug_logging_enabled(bool enable) noexcept { debug_logging_enabled_ = enable; }
    
    /**
     * @brief Set parent module for FastTrack logging context
     * @param parent Pointer to parent sc_module
     */
    void set_parent_module(sc_core::sc_module* parent) noexcept { parent_module_ = parent; }
    
    // Callback for when config registers change
    using ConfigChangeCallback = std::function<void()>;
    void set_change_callback(ConfigChangeCallback callback) { change_callback_ = callback; }
    
private:
    bool system_ready_;
    bool pcie_outbound_app_enable_;
    bool pcie_inbound_app_enable_;
    bool isolate_req_;
    bool debug_logging_enabled_;
    sc_core::sc_module* parent_module_;
    
    // SCML2 memory for config space with persistent storage
    scml2::memory<uint8_t> config_memory_;
    
    // Callback for config changes
    ConfigChangeCallback change_callback_;
    
    /**
     * @brief Process configuration read access
     * @param trans TLM2.0 generic payload
     * @param delay Temporal decoupling delay
     */
    void process_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    /**
     * @brief Process configuration write access
     * @param trans TLM2.0 generic payload
     * @param delay Temporal decoupling delay
     */
    void process_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    static const uint32_t SYSTEM_READY_OFFSET = 0x0FFFC;
    static const uint32_t PCIE_ENABLE_OFFSET = 0x0FFF8;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_CONFIG_REG_H
