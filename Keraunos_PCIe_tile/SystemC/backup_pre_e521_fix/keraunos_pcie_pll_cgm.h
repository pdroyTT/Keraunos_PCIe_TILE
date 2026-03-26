#ifndef KERAUNOS_PCIE_PLL_CGM_H
#define KERAUNOS_PCIE_PLL_CGM_H

// REFACTORED: C++ class with SCML2 memory

#include <scml2.h>
#include <scml2/memory.h>
#include <systemc>
#include <tlm>
#include <cstdint>

namespace keraunos {
namespace pcie {

class PllCgm {
public:
    PllCgm();
    ~PllCgm() = default;
    
    void process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_ref_clock(bool val) { ref_clock_ = val; }
    void set_reset_n(bool val) { reset_n_ = val; pll_locked_ = val; }
    
    bool get_pcie_clock() const { return pcie_clock_; }
    bool get_pll_lock() const { return pll_locked_; }
    
private:
    bool ref_clock_, reset_n_, pcie_clock_, pll_locked_;
    scml2::memory<uint8_t> pll_memory_;  // SCML2 memory
};

} // namespace pcie
} // namespace keraunos

#endif
