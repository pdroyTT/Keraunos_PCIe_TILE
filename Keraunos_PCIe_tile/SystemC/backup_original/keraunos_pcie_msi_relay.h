#ifndef KERAUNOS_PCIE_MSI_RELAY_H
#define KERAUNOS_PCIE_MSI_RELAY_H

#include <scml2.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <scml2/reg.h>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <sc_dt.h>
#include <vector>
#include <cstdint>

namespace keraunos {
namespace pcie {

/**
 * MSI Relay Unit
 * 
 * Supports N=16 MSI-X vectors
 * 
 * Main functionalities:
 * 1. Catching MSI from downstream and registering in PBA
 * 2. Picking pending interrupts from PBA and transferring upstream
 * 3. Providing CSR services
 */
class MsiRelayUnit : public sc_core::sc_module {
public:
    // APB Subordinate Port 1: CSR access (connected to upstream)
    tlm::tlm_base_target_socket<32> csr_apb_socket;
    
    // APB Subordinate Port 2: MSI receiver (connected to downstream)
    tlm::tlm_base_target_socket<32> msi_apb_socket;
    
    // AXI4-Lite Manager Port: MSI generation (to upstream)
    tlm_utils::simple_initiator_socket<MsiRelayUnit, 32> msi_axi_socket;
    
    // Control signals
    sc_core::sc_in<bool> msix_enable;      // MSI-X Enable from PCI Express Root Complex
    sc_core::sc_in<bool> msix_mask;         // MSI-X Mask from PCI Express Root Complex
    sc_core::sc_in<sc_dt::sc_bv<16> > setip;  // Interrupt Pending Signal (16-bit)
    
    SC_HAS_PROCESS(MsiRelayUnit);
    
    MsiRelayUnit(sc_core::sc_module_name name, uint8_t num_vectors = 16);
    virtual ~MsiRelayUnit();
    
    // CSR Register Access
    void write_msi_receiver(uint32_t data);
    uint32_t read_msi_outstanding() const;
    uint32_t read_msix_pba() const;
    void write_msix_table(uint8_t index, uint64_t address, uint32_t data, bool mask);
    void read_msix_table(uint8_t index, uint64_t& address, uint32_t& data, bool& mask) const;
    
protected:
    // SCML2 port adapters for APB sockets
    scml2::tlm2_gp_target_adapter<32> csr_adapter_;
    scml2::tlm2_gp_target_adapter<32> msi_adapter_;
    
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
    sc_dt::sc_bv<16> msix_pba_;
    
    // Outstanding MSI request count
    uint32_t msi_outstanding_;
    
    // SCML2 memory for CSR space (16KB)
    scml2::memory<uint8_t> csr_memory_;
    
    // SCML2 register objects for structured register access
    scml2::reg<uint32_t> msi_receiver_reg_;
    scml2::reg<uint32_t> msi_outstanding_reg_;
    scml2::reg<uint32_t> msix_pba_reg_;
    // Note: MSI-X table registers accessed directly via memory for now
    // Arrays of scml2::reg require special handling - can be added later if needed
    
    // TLM transport methods for APB ports (kept for msi_apb_socket special routing)
    tlm::tlm_sync_enum msi_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    
    // Callbacks for register writes to sync with internal data structures
    // According to VZ_SCMLRef.md Section 2.5.4, write callbacks use:
    // bool writeCallback(const DT& data, const DT& byteEnables, sc_core::sc_time&, int tag)
    bool msi_receiver_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool msix_table_addr_low_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool msix_table_addr_high_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool msix_table_data_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    
    // Callbacks for register reads from dynamic/internal data
    // According to VZ_SCMLRef.md Section 2.5.4, read callbacks use:
    // bool readCallback(DT& data, const DT& byteEnables, sc_core::sc_time&, int tag)
    bool msi_outstanding_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool msix_pba_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool msix_table_addr_low_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool msix_table_addr_high_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    bool msix_table_data_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t);
    
    // MSI thrower process
    void msi_thrower_process();
    
    // Helper methods
    void set_pba_bit(uint8_t index);
    void clear_pba_bit(uint8_t index);
    bool is_msi_allowed(uint8_t index) const;
    void send_msi(uint8_t index);
    
    // Address map constants
    static const uint32_t MSI_RECEIVER_OFFSET = 0x0000;
    static const uint32_t MSI_OUTSTANDING_OFFSET = 0x0004;
    static const uint32_t MSIX_PBA_OFFSET = 0x1000;
    static const uint32_t MSIX_TABLE_BASE_OFFSET = 0x2000;
    static const uint32_t MSIX_TABLE_ENTRY_SIZE = 16;  // 16 bytes per entry
};

} // namespace pcie
} // namespace keraunos

#endif // KERAUNOS_PCIE_MSI_RELAY_H

