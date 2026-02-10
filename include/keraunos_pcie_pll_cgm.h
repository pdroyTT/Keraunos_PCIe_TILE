#ifndef KERAUNOS_PCIE_PLL_CGM_H
#define KERAUNOS_PCIE_PLL_CGM_H

#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * PLL/CGM (Clock Generation Module)
 * 
 * Generates internal clocks from reference clock
 * Supports PLL lock status and configuration
 */
class PllCgm : public sc_core::sc_module {
public:
    // APB target socket for configuration
    tlm::tlm_base_target_socket<32> apb_socket;
    
    // Clock inputs
    sc_core::sc_in<bool> ref_clock;                    // Reference clock (100 MHz)
    
    // Clock outputs
    sc_core::sc_out<bool> pcie_clock;                  // Generated PCIE clock (1.0 GHz)
    
    // Status outputs
    sc_core::sc_out<bool> pll_lock;                    // PLL lock status
    
    // Control inputs
    sc_core::sc_in<bool> reset_n;                      // Reset (active low)
    
    SC_HAS_PROCESS(PllCgm);
    
    PllCgm(sc_core::sc_module_name name);
    virtual ~PllCgm();
    
    // Initialize outputs after elaboration (when ports are bound)
    virtual void end_of_elaboration();
    
protected:
    // SCML port adapter
    scml2::tlm2_gp_target_adapter<32> apb_adapter_;
    
    // SCML memory for PLL configuration
    scml2::memory<uint8_t> pll_memory_;
    
    // Clock generation process
    void clock_generation_process();
    
    // PLL lock process
    void pll_lock_process();
    
    // Clock periods
    sc_core::sc_time ref_clock_period_;
    sc_core::sc_time pcie_clock_period_;
    
    // Internal state
    bool pll_enabled_;
    bool pll_locked_;
    sc_core::sc_time lock_time_;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_PLL_CGM_H

