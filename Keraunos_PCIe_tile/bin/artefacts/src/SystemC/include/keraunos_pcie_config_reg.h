#ifndef KERAUNOS_PCIE_CONFIG_REG_H
#define KERAUNOS_PCIE_CONFIG_REG_H

// REFACTORED: C++ class with callback for value change notification

#include <scml2.h>
#include <scml2/memory.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <functional>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * Configuration Register Block (with callback for changes)
 */
class ConfigRegBlock {
public:
    ConfigRegBlock();
    ~ConfigRegBlock() = default;
    
    // Function interface (replaces apb_socket)
    void process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Status register outputs (const noexcept for performance, [[nodiscard]] to catch unused returns)
    [[nodiscard]] bool get_system_ready() const noexcept { return system_ready_; }
    [[nodiscard]] bool get_pcie_outbound_app_enable() const noexcept { return pcie_outbound_app_enable_; }
    [[nodiscard]] bool get_pcie_inbound_app_enable() const noexcept { return pcie_inbound_app_enable_; }
    
    // Control input (noexcept - no exceptions thrown)
    void set_isolate_req(const bool isolate) noexcept;
    
    // Callback for when config registers change
    using ConfigChangeCallback = std::function<void()>;
    void set_change_callback(ConfigChangeCallback callback) { change_callback_ = callback; }
    
private:
    bool system_ready_;
    bool pcie_outbound_app_enable_;
    bool pcie_inbound_app_enable_;
    bool isolate_req_;
    
    // SCML2 memory for config space with persistent storage
    scml2::memory<uint8_t> config_memory_;
    
    // Callback for config changes
    ConfigChangeCallback change_callback_;
    
    void process_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    static const uint32_t SYSTEM_READY_OFFSET = 0x0FFFC;
    static const uint32_t PCIE_ENABLE_OFFSET = 0x0FFF8;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_CONFIG_REG_H
