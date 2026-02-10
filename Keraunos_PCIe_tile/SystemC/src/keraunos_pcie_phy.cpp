#include "keraunos_pcie_phy.h"
#include <cstring>

namespace keraunos {
namespace pcie {

PciePhy::PciePhy()
    : reset_n_(false), ref_clock_(false), phy_ready_(false)
    , phy_memory_("phy_memory", 65536)  // 64KB SCML2 memory
{
}

void PciePhy::process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        if (offset + len <= phy_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                data_ptr[i] = phy_memory_[offset + i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        if (offset + len <= phy_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                phy_memory_[offset + i] = data_ptr[i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void PciePhy::process_ahb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    process_apb_access(trans, delay);
}

} // namespace pcie
} // namespace keraunos
