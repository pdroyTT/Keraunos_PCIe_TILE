#ifndef KERAUNOS_PCIE_PHY_H
#define KERAUNOS_PCIE_PHY_H

// REFACTORED: C++ class with SCML2 memory

#include <scml2.h>
#include <scml2/memory.h>
#include <systemc>
#include <tlm>
#include <cstdint>

namespace keraunos {
namespace pcie {

class PciePhy {
public:
    PciePhy();
    ~PciePhy() = default;
    
    void process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_ahb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void set_reset_n(bool val) { reset_n_ = val; phy_ready_ = val; }
    void set_ref_clock(bool val) { ref_clock_ = val; }
    
    bool get_phy_ready() const { return phy_ready_; }
    
private:
    bool reset_n_, ref_clock_, phy_ready_;
    scml2::memory<uint8_t> phy_memory_;  // SCML2 memory
};

} // namespace pcie
} // namespace keraunos

#endif
