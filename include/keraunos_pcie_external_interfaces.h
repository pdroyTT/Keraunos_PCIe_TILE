#ifndef KERAUNOS_PCIE_EXTERNAL_INTERFACES_H
#define KERAUNOS_PCIE_EXTERNAL_INTERFACES_H

#include <scml2.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <sc_dt.h>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * NOC-N Interface
 * 
 * External interface to NOC network
 * - 256-bit data width
 * - 52-bit address width (masked to 64-bit for SCML2 compatibility)
 */
class NocNInterface : public sc_core::sc_module {
public:
    // AXI4 target socket (from NOC-IO switch)
    // Using 64-bit socket, addresses masked to 52 bits
    tlm_utils::simple_target_socket<NocNInterface, 64> target_socket;
    
    // AXI4 initiator socket (to external NOC)
    scml2::initiator_socket<64> initiator_socket;
    
    SC_HAS_PROCESS(NocNInterface);
    
    NocNInterface(sc_core::sc_module_name name);
    virtual ~NocNInterface();
    
protected:
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi);
};

/**
 * SMN-N Interface
 * 
 * External interface to SMN network
 * - 64-bit data width
 * - 52-bit address width (masked to 64-bit for SCML2 compatibility)
 */
class SmnNInterface : public sc_core::sc_module {
public:
    // AXI4 target socket (from SMN-IO switch)
    // Using 64-bit socket, addresses masked to 52 bits
    tlm_utils::simple_target_socket<SmnNInterface, 64> target_socket;
    
    // AXI4 initiator socket (to external SMN)
    scml2::initiator_socket<64> initiator_socket;
    
    SC_HAS_PROCESS(SmnNInterface);
    
    SmnNInterface(sc_core::sc_module_name name);
    virtual ~SmnNInterface();
    
protected:
    // TLM transport methods
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi);
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_EXTERNAL_INTERFACES_H

