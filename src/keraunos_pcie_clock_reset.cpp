#include "keraunos_pcie_clock_reset.h"

namespace keraunos {
namespace pcie {

ClockResetControl::ClockResetControl(sc_core::sc_module_name name)
    : sc_module(name)
    , pcie_clock_period_(sc_core::sc_time(1.0, sc_core::SC_NS))  // 1.0 GHz = 1ns period
    , ref_clock_period_(sc_core::sc_time(10.0, sc_core::SC_NS))   // 100 MHz = 10ns period
    , pcie_clock_enable_(false)
    , ref_clock_enable_(true)  // Reference clock always enabled
{
    // Register clock generation processes
    SC_THREAD(pcie_clock_process);
    SC_THREAD(ref_clock_process);
    
    // Register reset control process
    SC_METHOD(reset_control_process);
    sensitive << cold_reset_n << warm_reset_n << isolate_req;
    
    // Note: Output initialization moved to end_of_elaboration() 
    // to avoid writing to unbound ports during construction
}

ClockResetControl::~ClockResetControl() {
}

void ClockResetControl::end_of_elaboration() {
    sc_module::end_of_elaboration();
    
    // Initialize outputs now that all ports are bound
    pcie_clock.write(false);
    ref_clock.write(false);
    pcie_sii_reset_ctrl.write(true);   // Active low, so 1 = deasserted
    pcie_reset_ctrl.write(true);
    force_to_ref_clk_n.write(true);
}

void ClockResetControl::pcie_clock_process() {
    while (true) {
        if (pcie_clock_enable_ && !isolate_req.read()) {
            pcie_clock.write(!pcie_clock.read());
            wait(pcie_clock_period_ / 2);
        } else {
            pcie_clock.write(false);
            wait(pcie_clock_period_);
        }
    }
}

void ClockResetControl::ref_clock_process() {
    while (true) {
        if (ref_clock_enable_ && !isolate_req.read()) {
            ref_clock.write(!ref_clock.read());
            wait(ref_clock_period_ / 2);
        } else {
            ref_clock.write(false);
            wait(ref_clock_period_);
        }
    }
}

void ClockResetControl::reset_control_process() {
    // Cold reset affects both SII and main reset
    if (!cold_reset_n.read()) {
        pcie_sii_reset_ctrl.write(false);  // Assert reset
        pcie_reset_ctrl.write(false);
        pcie_clock_enable_ = false;
    } else {
        pcie_sii_reset_ctrl.write(true);   // Deassert reset
        // Warm reset only affects main reset
        if (!warm_reset_n.read()) {
            pcie_reset_ctrl.write(false);
        } else {
            pcie_reset_ctrl.write(true);
            // Enable PCIE clock after reset deassertion
            pcie_clock_enable_ = true;
        }
    }
    
    // Force to reference clock control
    force_to_ref_clk_n.write(pcie_clock_enable_);
}

} // namespace pcie
} // namespace keraunos

