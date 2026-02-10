#ifndef KERAUNOS_PCIE_CLOCK_RESET_H
#define KERAUNOS_PCIE_CLOCK_RESET_H

#include "keraunos_pcie_common.h"
#include <systemc>
#include <sc_dt.h>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * Clock & Reset Control Module
 * 
 * Manages clock and reset signals for PCIE Tile
 * Supports Cold Reset, Warm Reset, and Isolation
 */
class ClockResetControl : public sc_core::sc_module {
public:
    // Input reset signals
    sc_core::sc_in<bool> cold_reset_n;                  // Cold reset (Management + Main)
    sc_core::sc_in<bool> warm_reset_n;                   // Warm reset (Main only)
    sc_core::sc_in<bool> isolate_req;                   // Isolation request
    
    // Output reset signals
    sc_core::sc_out<bool> pcie_sii_reset_ctrl;          // SII reset control
    sc_core::sc_out<bool> pcie_reset_ctrl;              // Main reset control
    sc_core::sc_out<bool> force_to_ref_clk_n;           // Force to reference clock
    
    // Clock outputs
    sc_core::sc_out<bool> pcie_clock;                   // PCIE clock (1.0 GHz)
    sc_core::sc_out<bool> ref_clock;                    // Reference clock (100 MHz)
    
    SC_HAS_PROCESS(ClockResetControl);
    
    ClockResetControl(sc_core::sc_module_name name);
    virtual ~ClockResetControl();
    
    // Initialize outputs after elaboration (when ports are bound)
    virtual void end_of_elaboration();
    
protected:
    // Clock generation processes
    void pcie_clock_process();
    void ref_clock_process();
    
    // Reset control process
    void reset_control_process();
    
    // Clock periods
    sc_core::sc_time pcie_clock_period_;
    sc_core::sc_time ref_clock_period_;
    
    // Internal state
    bool pcie_clock_enable_;
    bool ref_clock_enable_;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_CLOCK_RESET_H

