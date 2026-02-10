#include "keraunos_pcie_clock_reset.h"

namespace keraunos {
namespace pcie {

ClockResetControl::ClockResetControl() noexcept
    : cold_reset_n_(false), warm_reset_n_(false), isolate_req_(false)
    , pcie_sii_reset_ctrl_(false), pcie_reset_ctrl_(false), force_to_ref_clk_n_(true)
    , pcie_clock_(true), ref_clock_(true)
{}

void ClockResetControl::update_resets() noexcept {
    pcie_sii_reset_ctrl_ = cold_reset_n_;
    pcie_reset_ctrl_ = cold_reset_n_ && warm_reset_n_;
    force_to_ref_clk_n_ = !isolate_req_;
}

} // namespace pcie
} // namespace keraunos
