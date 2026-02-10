#ifndef KERAUNOS_PCIE_CLOCK_RESET_H
#define KERAUNOS_PCIE_CLOCK_RESET_H

// REFACTORED: C++ class

#include "keraunos_pcie_common.h"
#include <systemc>
#include <cstdint>

namespace keraunos {
namespace pcie {

class ClockResetControl {
public:
    ClockResetControl() noexcept;
    ~ClockResetControl() = default;
    
    void set_cold_reset_n(const bool val) noexcept { cold_reset_n_ = val; update_resets(); }
    void set_warm_reset_n(const bool val) noexcept { warm_reset_n_ = val; update_resets(); }
    void set_isolate_req(const bool val) noexcept { isolate_req_ = val; update_resets(); }
    
    [[nodiscard]] bool get_pcie_sii_reset_ctrl() const noexcept { return pcie_sii_reset_ctrl_; }
    [[nodiscard]] bool get_pcie_reset_ctrl() const noexcept { return pcie_reset_ctrl_; }
    [[nodiscard]] bool get_force_to_ref_clk_n() const noexcept { return force_to_ref_clk_n_; }
    [[nodiscard]] bool get_pcie_clock() const noexcept { return pcie_clock_; }
    [[nodiscard]] bool get_ref_clock() const noexcept { return ref_clock_; }
    
private:
    bool cold_reset_n_, warm_reset_n_, isolate_req_;
    bool pcie_sii_reset_ctrl_, pcie_reset_ctrl_, force_to_ref_clk_n_;
    bool pcie_clock_, ref_clock_;
    
    void update_resets() noexcept;
};

} // namespace pcie
} // namespace keraunos

#endif
