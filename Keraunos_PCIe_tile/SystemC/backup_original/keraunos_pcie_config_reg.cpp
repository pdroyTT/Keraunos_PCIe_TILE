#include "keraunos_pcie_config_reg.h"
#include <cstring>

namespace keraunos {
namespace pcie {

ConfigRegBlock::ConfigRegBlock(sc_core::sc_module_name name)
    : sc_module(name)
    , apb_adapter_("apb_adapter", apb_socket)
    , config_memory_("config_memory", 64 * 1024)  // 64KB for TLB config + status
    , system_ready_reg_("system_ready", config_memory_, 0x0FFFC)
    , pcie_enable_reg_("pcie_enable", config_memory_, 0x0FFF8)
{
    // Bind SCML adapter to memory
    apb_adapter_(config_memory_);
    
    // Configure register callbacks
    // TODO: Fix callback API - write_adapter signature doesn't match callback system
    // Temporarily disabled - callbacks need to be implemented using correct SCML2 API
    // system_ready_reg_.set_write_callback(...);
    // pcie_enable_reg_.set_write_callback(...);
    
    // Register isolation process
    SC_METHOD(isolation_process);
    sensitive << isolate_req;
    
    // Note: Output initialization moved to end_of_elaboration() 
    // to avoid writing to unbound ports during construction
}

ConfigRegBlock::~ConfigRegBlock() {
}

void ConfigRegBlock::end_of_elaboration() {
    sc_module::end_of_elaboration();
    
    // Initialize outputs now that all ports are bound
    system_ready.write(false);
    pcie_outbound_app_enable.write(false);
    pcie_inbound_app_enable.write(false);
}

bool ConfigRegBlock::system_ready_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // System Ready register: bit[0]
    bool ready = (data & 0x1) != 0;
    system_ready.write(ready);
    return true;
}

bool ConfigRegBlock::pcie_enable_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // PCIE Enable register: bit[0] = outbound, bit[16] = inbound
    bool outbound_enable = (data & 0x1) != 0;
    bool inbound_enable = (data & 0x10000) != 0;
    
    pcie_outbound_app_enable.write(outbound_enable);
    pcie_inbound_app_enable.write(inbound_enable);
    return true;
}

void ConfigRegBlock::isolation_process() {
    // When isolate_req is asserted, clear enable bits
    if (isolate_req.read()) {
        system_ready.write(false);
        pcie_outbound_app_enable.write(false);
        pcie_inbound_app_enable.write(false);
    }
}

} // namespace pcie
} // namespace keraunos

