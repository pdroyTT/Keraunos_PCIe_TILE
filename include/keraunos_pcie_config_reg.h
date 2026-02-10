#ifndef KERAUNOS_PCIE_CONFIG_REG_H
#define KERAUNOS_PCIE_CONFIG_REG_H

#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <scml2/reg.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * Configuration Register Block
 * 
 * Contains TLB configuration registers and status registers
 * - TLB configuration space (Bank-0)
 * - Status registers (System Ready, PCIE Enable bits)
 */
class ConfigRegBlock : public sc_core::sc_module {
public:
    // APB target socket
    tlm::tlm_base_target_socket<32> apb_socket;
    
    // Status register outputs
    sc_core::sc_out<bool> system_ready;                 // System Ready bit
    sc_core::sc_out<bool> pcie_outbound_app_enable;     // PCIE Outbound Enable
    sc_core::sc_out<bool> pcie_inbound_app_enable;      // PCIE Inbound Enable
    
    // Control inputs
    sc_core::sc_in<bool> isolate_req;                  // Isolation request
    
    SC_HAS_PROCESS(ConfigRegBlock);
    
    ConfigRegBlock(sc_core::sc_module_name name);
    virtual ~ConfigRegBlock();
    
    // Initialize outputs after elaboration (when ports are bound)
    virtual void end_of_elaboration();
    
protected:
    // SCML port adapter
    scml2::tlm2_gp_target_adapter<32> apb_adapter_;
    
    // SCML memory for config register space
    scml2::memory<uint8_t> config_memory_;
    
    // Status registers
    scml2::reg<uint32_t> system_ready_reg_;            // Offset 0x0FFFC
    scml2::reg<uint32_t> pcie_enable_reg_;              // Offset 0x0FFF8
    
    // Register callbacks
    bool system_ready_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool pcie_enable_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    
    // Isolation handling
    void isolation_process();
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_CONFIG_REG_H

