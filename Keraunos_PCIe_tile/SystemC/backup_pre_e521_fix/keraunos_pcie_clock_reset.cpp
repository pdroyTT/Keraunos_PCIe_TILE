#include "keraunos_pcie_clock_reset.h"

namespace keraunos {
namespace pcie {

ClockResetControl::ClockResetControl() noexcept
    : cold_reset_n_(false), warm_reset_n_(false), isolate_req_(false)
    , pcie_sii_reset_ctrl_(false), pcie_reset_ctrl_(false), force_to_ref_clk_n_(true)
    , pcie_clock_(true), ref_clock_(true)
    , debug_logging_enabled_(true), parent_module_(nullptr)
{}

void ClockResetControl::update_resets() noexcept {
    bool prev_sii_reset = pcie_sii_reset_ctrl_;
    bool prev_pcie_reset = pcie_reset_ctrl_;
    bool prev_force_ref = force_to_ref_clk_n_;
    
    pcie_sii_reset_ctrl_ = cold_reset_n_;
    pcie_reset_ctrl_ = cold_reset_n_ && warm_reset_n_;
    force_to_ref_clk_n_ = !isolate_req_;
    
    if (debug_logging_enabled_ && parent_module_ &&
        (prev_sii_reset != pcie_sii_reset_ctrl_ || 
         prev_pcie_reset != pcie_reset_ctrl_ || 
         prev_force_ref != force_to_ref_clk_n_)) {
        SCML2_INFO_TO(parent_module_, CONFIGURATION_INFO) 
            << "Reset state updated: cold_reset_n=" << cold_reset_n_
            << " warm_reset_n=" << warm_reset_n_
            << " isolate=" << isolate_req_
            << " -> sii_reset=" << pcie_sii_reset_ctrl_
            << " pcie_reset=" << pcie_reset_ctrl_
            << " force_ref=" << force_to_ref_clk_n_ << std::endl;
    }
}

} // namespace pcie
} // namespace keraunos
