#include "keraunos_pcie_pll_cgm.h"
#include <cstring>

namespace keraunos {
namespace pcie {

PllCgm::PllCgm()
    : ref_clock_(false), reset_n_(false), pcie_clock_(false), pll_locked_(false)
    , pll_memory_("pll_memory", 4096)  // SCML2 memory
{
}

void PllCgm::process_apb_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t len = trans.get_data_length();
    uint8_t* data_ptr = trans.get_data_ptr();
    
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        if (offset + len <= pll_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                data_ptr[i] = pll_memory_[offset + i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        if (offset + len <= pll_memory_.get_size()) {
            for (uint32_t i = 0; i < len; i++) {
                pll_memory_[offset + i] = data_ptr[i];
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

} // namespace pcie
} // namespace keraunos
