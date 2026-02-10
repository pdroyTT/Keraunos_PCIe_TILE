#ifndef KERAUNOS_PCIE_MSI_RELAY_H
#define KERAUNOS_PCIE_MSI_RELAY_H

// REFACTORED: Converted from sc_module to pure C++ class
// Original backed up in SystemC/backup_original/

#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <vector>
#include <functional>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * MSI Relay Unit (Refactored to C++ Class)
 * No sc_module, no TLM sockets - function-based communication only
 */
class MsiRelayUnit {
public:
    explicit MsiRelayUnit(uint8_t num_vectors = 16);
    ~MsiRelayUnit() = default;
    
    // Function interfaces (replace sockets)
    void process_csr_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_msi_input(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    void set_msi_output_callback(TransportCallback callback);
    
    // Control signal interfaces
    void set_msix_enable(bool enable);
    void set_msix_mask(bool mask);
    void set_interrupt_pending(uint16_t setip_bits);
    
    // Register access
    void write_msi_receiver(uint32_t data);
    uint32_t read_msi_outstanding() const;
    uint32_t read_msix_pba() const;
    void write_msix_table(uint8_t index, uint64_t address, uint32_t data, bool mask);
    void read_msix_table(uint8_t index, uint64_t& address, uint32_t& data, bool& mask) const;
    
    // Processing (replaces SC_THREAD)
    void process_pending_msis();
    
private:
    const uint8_t num_vectors_;
    
    struct MsixTableEntry {
        uint64_t address;
        uint32_t data;
        bool mask;
        MsixTableEntry() : address(0), data(0), mask(true) {}
    };
    
    std::vector<MsixTableEntry> msix_table_;
    uint16_t msix_pba_;
    uint32_t msi_outstanding_;
    bool msix_enable_;
    bool msix_mask_;
    uint16_t setip_;
    TransportCallback msi_output_callback_;
    
    void set_pba_bit(uint8_t index);
    void clear_pba_bit(uint8_t index);
    bool is_msi_allowed(uint8_t index) const;
    void send_msi(uint8_t index);
    void process_csr_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_csr_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    static const uint32_t MSI_RECEIVER_OFFSET = 0x0000;
    static const uint32_t MSI_OUTSTANDING_OFFSET = 0x0004;
    static const uint32_t MSIX_PBA_OFFSET = 0x1000;
    static const uint32_t MSIX_TABLE_BASE_OFFSET = 0x2000;
    static const uint32_t MSIX_TABLE_ENTRY_SIZE = 16;
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_MSI_RELAY_H
