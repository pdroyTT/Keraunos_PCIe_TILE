#ifndef KERAUNOS_PCIE_PHY_H
#define KERAUNOS_PCIE_PHY_H

#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * PCI Express PHY Model (SerDes)
 * 
 * High-level abstraction of Synopsys PCIE PHY IP (Gen6 x4)
 * Supports lane reversal and configuration via APB/AHB
 */
class PciePhy : public sc_core::sc_module {
public:
    // APB target socket for configuration
    tlm::tlm_base_target_socket<32> apb_socket;
    
    // AHB target socket for firmware download
    tlm::tlm_base_target_socket<32> ahb_socket;
    
    // Control signals
    sc_core::sc_in<bool> reset_n;                       // Reset (active low)
    sc_core::sc_in<bool> ref_clock;                     // Reference clock
    
    // Status outputs
    sc_core::sc_out<bool> phy_ready;                   // PHY ready status
    
    SC_HAS_PROCESS(PciePhy);
    
    PciePhy(sc_core::sc_module_name name);
    virtual ~PciePhy();
    
    // Initialize outputs after elaboration (when ports are bound)
    virtual void end_of_elaboration();
    
protected:
    // SCML port adapters
    scml2::tlm2_gp_target_adapter<32> apb_adapter_;
    scml2::tlm2_gp_target_adapter<32> ahb_adapter_;
    
    // SCML memory for PHY configuration
    scml2::memory<uint8_t> phy_memory_;
    
    // Lane reversal support
    bool lane_reversal_enabled_;
    
    // Initialization process
    void initialization_process();
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_PHY_H

