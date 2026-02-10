#include "keraunos_pcie_phy.h"
#include <cstring>

namespace keraunos {
namespace pcie {

PciePhy::PciePhy(sc_core::sc_module_name name)
    : sc_module(name)
    , apb_adapter_("apb_adapter", apb_socket)
    , ahb_adapter_("ahb_adapter", ahb_socket)
    , phy_memory_("phy_memory", 256 * 1024)  // 256KB for SerDes config
    , lane_reversal_enabled_(false)
{
    // Bind SCML adapters to memory
    apb_adapter_(phy_memory_);
    ahb_adapter_(phy_memory_);
    
    // Register initialization process
    SC_METHOD(initialization_process);
    sensitive << reset_n;
    
    // Note: Output initialization moved to end_of_elaboration() 
    // to avoid writing to unbound ports during construction
}

PciePhy::~PciePhy() {
}

void PciePhy::end_of_elaboration() {
    sc_module::end_of_elaboration();
    
    // Initialize outputs now that all ports are bound
    phy_ready.write(false);
}

void PciePhy::initialization_process() {
    if (reset_n.read()) {
        // PHY initialization sequence
        // In a real implementation, this would wait for firmware download
        // and configuration completion
        wait(sc_core::sc_time(100, sc_core::SC_US));  // Simulate init time
        phy_ready.write(true);
    } else {
        phy_ready.write(false);
    }
}

} // namespace pcie
} // namespace keraunos

