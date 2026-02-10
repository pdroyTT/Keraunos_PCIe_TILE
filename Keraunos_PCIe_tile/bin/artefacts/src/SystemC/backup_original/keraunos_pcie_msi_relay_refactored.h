#ifndef KERAUNOS_PCIE_MSI_RELAY_REFACTORED_H
#define KERAUNOS_PCIE_MSI_RELAY_REFACTORED_H

#include <systemc>
#include <tlm>
#include <sc_dt.h>
#include <vector>
#include <functional>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * MSI Relay Unit (Refactored - C++ Class)
 * 
 * Supports N=16 MSI-X vectors
 * 
 * Main functionalities:
 * 1. Catching MSI from downstream and registering in PBA
 * 2. Picking pending interrupts from PBA and transferring upstream
 * 3. Providing CSR services
 * 
 * REFACTORED: No sc_module, no sockets - pure C++ class with function interfaces
 */
class MsiRelayRefactored {
public:
    // Constructor
    explicit MsiRelayRefactored(uint8_t num_vectors = 16);
    ~MsiRelayRefactored() = default;
    
    // ========================================================================
    // Function Interfaces (replace sockets)
    // ========================================================================
    
    // CSR Access Interface (replaces csr_apb_socket target)
    void process_csr_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // MSI Receiver Interface (replaces msi_apb_socket target)
    void process_msi_input(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Set callback for MSI output (replaces msi_axi_socket initiator)
    using TransportCallback = std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)>;
    void set_msi_output_callback(TransportCallback callback);
    
    // ========================================================================
    // Control Signal Interfaces (replace sc_in signals)
    // ========================================================================
    
    void set_msix_enable(bool enable);
    void set_msix_mask(bool mask);
    void set_interrupt_pending(uint16_t setip_bits);
    
    // ========================================================================
    // Register Access Methods (direct interface)
    // ========================================================================
    
    void write_msi_receiver(uint32_t data);
    uint32_t read_msi_outstanding() const;
    uint32_t read_msix_pba() const;
    void write_msix_table(uint8_t index, uint64_t address, uint32_t data, bool mask);
    void read_msix_table(uint8_t index, uint64_t& address, uint32_t& data, bool& mask) const;
    
    // ========================================================================
    // Processing Method (called by parent to trigger MSI sending)
    // ========================================================================
    
    // Call this when signals change to process pending MSIs
    void process_pending_msis();
    
private:
    // Number of MSI-X vectors (default 16)
    const uint8_t num_vectors_;
    
    // MSI-X Table Entry Structure
    struct MsixTableEntry {
        uint64_t address;    // [63:2] MSI Address
        uint32_t data;       // [95:64] MSI Data
        bool mask;           // [96] Mask bit
        
        MsixTableEntry() : address(0), data(0), mask(true) {}
    };
    
    // MSI-X Table (N entries)
    std::vector<MsixTableEntry> msix_table_;
    
    // Pending Bit Array (PBA) - one bit per vector
    uint16_t msix_pba_;
    
    // Outstanding MSI request count
    uint32_t msi_outstanding_;
    
    // Control signals (stored internally)
    bool msix_enable_;
    bool msix_mask_;
    uint16_t setip_;
    
    // Callback for MSI output
    TransportCallback msi_output_callback_;
    
    // Helper methods
    void set_pba_bit(uint8_t index);
    void clear_pba_bit(uint8_t index);
    bool is_msi_allowed(uint8_t index) const;
    void send_msi(uint8_t index);
    
    // CSR processing helpers
    void process_csr_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    void process_csr_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Address map constants
    static const uint32_t MSI_RECEIVER_OFFSET = 0x0000;
    static const uint32_t MSI_OUTSTANDING_OFFSET = 0x0004;
    static const uint32_t MSIX_PBA_OFFSET = 0x1000;
    static const uint32_t MSIX_TABLE_BASE_OFFSET = 0x2000;
    static const uint32_t MSIX_TABLE_ENTRY_SIZE = 16;  // 16 bytes per entry
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_MSI_RELAY_REFACTORED_H
