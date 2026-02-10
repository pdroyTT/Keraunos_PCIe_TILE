#include "keraunos_pcie_pll_cgm.h"
#include <cstring>

namespace keraunos {
namespace pcie {

PllCgm::PllCgm(sc_core::sc_module_name name)
    : sc_module(name)
    , apb_adapter_("apb_adapter", apb_socket)
    , pll_memory_("pll_memory", 4 * 1024)  // 4KB for PLL config
    , ref_clock_period_(sc_core::sc_time(10.0, sc_core::SC_NS))   // 100 MHz
    , pcie_clock_period_(sc_core::sc_time(1.0, sc_core::SC_NS))   // 1.0 GHz
    , pll_enabled_(false)
    , pll_locked_(false)
    , lock_time_(sc_core::sc_time(170, sc_core::SC_NS))  // 170 ref clock cycles
{
    // Bind SCML adapter to memory
    apb_adapter_(pll_memory_);
    
    // Register processes
    SC_THREAD(clock_generation_process);
    SC_THREAD(pll_lock_process);
    
    // Note: Output initialization moved to end_of_elaboration() 
    // to avoid writing to unbound ports during construction
}

PllCgm::~PllCgm() {
}

void PllCgm::end_of_elaboration() {
    sc_module::end_of_elaboration();
    
    // Initialize outputs now that all ports are bound
    pcie_clock.write(false);
    pll_lock.write(false);
}

void PllCgm::clock_generation_process() {
    while (true) {
        if (pll_enabled_ && pll_locked_ && reset_n.read()) {
            pcie_clock.write(!pcie_clock.read());
            wait(pcie_clock_period_ / 2);
        } else {
            pcie_clock.write(false);
            wait(ref_clock_period_);
        }
    }
}

void PllCgm::pll_lock_process() {
    while (true) {
        wait();  // Wait for reset or enable changes
        
        if (reset_n.read() && pll_enabled_) {
            // Wait for lock time (170 ref clock cycles)
            wait(lock_time_);
            pll_locked_ = true;
            pll_lock.write(true);
        } else {
            pll_locked_ = false;
            pll_lock.write(false);
        }
    }
}

} // namespace pcie
} // namespace keraunos

