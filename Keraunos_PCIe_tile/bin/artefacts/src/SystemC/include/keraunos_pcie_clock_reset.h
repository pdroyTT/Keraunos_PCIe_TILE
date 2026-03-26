#ifndef KERAUNOS_PCIE_CLOCK_RESET_H
#define KERAUNOS_PCIE_CLOCK_RESET_H

// REFACTORED: C++ class

#include "keraunos_pcie_common.h"
#include "scml2/tagged_message_macros.h"
#include <systemc>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * @brief Clock and Reset Control for PCIe Tile
 * 
 * Manages clock gating and reset signal generation based on
 * cold reset, warm reset, and isolation request inputs.
 */
class ClockResetControl {
public:
    ClockResetControl() noexcept;
    ~ClockResetControl() = default;
    
    /**
     * @brief Set cold reset state (active low)
     * @param val Cold reset signal (false = asserted)
     */
    void set_cold_reset_n(const bool val) noexcept { cold_reset_n_ = val; update_resets(); }
    
    /**
     * @brief Set warm reset state (active low)
     * @param val Warm reset signal (false = asserted)
     */
    void set_warm_reset_n(const bool val) noexcept { warm_reset_n_ = val; update_resets(); }
    
    /**
     * @brief Set isolation request
     * @param val Isolation request (true = isolate)
     */
    void set_isolate_req(const bool val) noexcept { isolate_req_ = val; update_resets(); }
    
    /**
     * @brief Get PCIe SII reset control output
     * @return Reset control for SII block
     */
    [[nodiscard]] bool get_pcie_sii_reset_ctrl() const noexcept { return pcie_sii_reset_ctrl_; }
    
    /**
     * @brief Get PCIe reset control output
     * @return Reset control for PCIe logic
     */
    [[nodiscard]] bool get_pcie_reset_ctrl() const noexcept { return pcie_reset_ctrl_; }
    
    /**
     * @brief Get force to reference clock signal
     * @return Clock select signal (active low)
     */
    [[nodiscard]] bool get_force_to_ref_clk_n() const noexcept { return force_to_ref_clk_n_; }
    
    /**
     * @brief Get PCIe clock state
     * @return PCIe clock signal
     */
    [[nodiscard]] bool get_pcie_clock() const noexcept { return pcie_clock_; }
    
    /**
     * @brief Get reference clock state
     * @return Reference clock signal
     */
    [[nodiscard]] bool get_ref_clock() const noexcept { return ref_clock_; }
    
    /**
     * @brief Enable or disable debug logging
     * @param enable true to enable FastTrack debug logging
     */
    void set_debug_logging_enabled(bool enable) noexcept { debug_logging_enabled_ = enable; }
    
    /**
     * @brief Set parent module for FastTrack logging context
     * @param parent Pointer to parent sc_module
     */
    void set_parent_module(sc_core::sc_module* parent) noexcept { parent_module_ = parent; }
    
private:
    bool cold_reset_n_, warm_reset_n_, isolate_req_;
    bool pcie_sii_reset_ctrl_, pcie_reset_ctrl_, force_to_ref_clk_n_;
    bool pcie_clock_, ref_clock_;
    bool debug_logging_enabled_;
    sc_core::sc_module* parent_module_;
    
    /**
     * @brief Update derived reset signals based on input states
     */
    void update_resets() noexcept;
};

} // namespace pcie
} // namespace keraunos

#endif
