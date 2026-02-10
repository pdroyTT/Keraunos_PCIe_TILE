#include "keraunos_pcie_external_interfaces.h"
#include "keraunos_pcie_common.h"
#include <cstring>

namespace keraunos {
namespace pcie {

//=============================================================================
// NOC-N Interface Implementation
//=============================================================================

NocNInterface::NocNInterface(sc_core::sc_module_name name)
    : sc_module(name)
{
    // Register transport methods
    target_socket.register_b_transport(this, &NocNInterface::b_transport);
    target_socket.register_transport_dbg(this, &NocNInterface::transport_dbg);
    target_socket.register_get_direct_mem_ptr(this, &NocNInterface::get_direct_mem_ptr);
}

NocNInterface::~NocNInterface() {
}

void NocNInterface::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Mask address to 52 bits before forwarding
    // NOC-N uses 52-bit addresses, but socket is 64-bit for SCML2 compatibility
    set_52bit_address(trans, trans.get_address());
    
    // Forward transaction to external NOC using simple_initiator_socket
    // Use operator->() to access the TLM interface
    initiator_socket->b_transport(trans, delay);
}

unsigned int NocNInterface::transport_dbg(tlm::tlm_generic_payload& trans) {
    // Use operator->() to access the TLM interface
    return initiator_socket->transport_dbg(trans);
}

bool NocNInterface::get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi) {
    return initiator_socket->get_direct_mem_ptr(trans, dmi);
}

//=============================================================================
// SMN-N Interface Implementation
//=============================================================================

SmnNInterface::SmnNInterface(sc_core::sc_module_name name)
    : sc_module(name)
{
    // Register transport methods
    target_socket.register_b_transport(this, &SmnNInterface::b_transport);
    target_socket.register_transport_dbg(this, &SmnNInterface::transport_dbg);
    target_socket.register_get_direct_mem_ptr(this, &SmnNInterface::get_direct_mem_ptr);
}

SmnNInterface::~SmnNInterface() {
}

void SmnNInterface::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // Mask address to 52 bits before forwarding
    // SMN-N uses 52-bit addresses, but socket is 64-bit for SCML2 compatibility
    set_52bit_address(trans, trans.get_address());
    
    // Forward transaction to external SMN using simple_initiator_socket
    // Use operator->() to access the TLM interface
    initiator_socket->b_transport(trans, delay);
}

unsigned int SmnNInterface::transport_dbg(tlm::tlm_generic_payload& trans) {
    // Mask address to 52 bits before forwarding
    set_52bit_address(trans, trans.get_address());
    
    // Use operator->() to access the TLM interface
    return initiator_socket->transport_dbg(trans);
}

bool SmnNInterface::get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi) {
    return initiator_socket->get_direct_mem_ptr(trans, dmi);
}

} // namespace pcie
} // namespace keraunos

