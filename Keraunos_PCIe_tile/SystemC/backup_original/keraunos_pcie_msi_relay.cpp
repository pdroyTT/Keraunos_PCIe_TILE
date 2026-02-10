#include "keraunos_pcie_msi_relay.h"
#include <cstring>
#include <iomanip>

namespace keraunos {
namespace pcie {

MsiRelayUnit::MsiRelayUnit(sc_core::sc_module_name name, uint8_t num_vectors)
    : sc_module(name)
    , csr_adapter_("csr_adapter", csr_apb_socket)
    , msi_adapter_("msi_adapter", msi_apb_socket)
    , num_vectors_(num_vectors)
    , msix_table_(num_vectors)
    , msix_pba_(0)
    , msi_outstanding_(0)
    , csr_memory_("csr_memory", 16 * 1024)  // 16KB CSR space
    , msi_receiver_reg_("msi_receiver", csr_memory_, 0x0000)
    , msi_outstanding_reg_("msi_outstanding", csr_memory_, 0x0004)
    , msix_pba_reg_("msix_pba", csr_memory_, 0x1000)
{
    // Initialize MSI-X table entries
    for (auto& entry : msix_table_) {
        entry.address = 0;
        entry.data = 0;
        entry.mask = true;  // Masked by default
    }
    
    // Initialize PBA
    msix_pba_ = 0;
    
    // Bind SCML adapter to memory
    csr_adapter_(csr_memory_);
    
    // MSI-X table registers accessed directly via memory
    // Register objects can be added later if structured access is needed
    
    // Configure register callbacks using SCML register callback API
    // According to VZ_SCMLRef.md Section 2.5.4, use set_write_callback/set_read_callback with SCML2_CALLBACK macro
    scml2::set_write_callback(msi_receiver_reg_, SCML2_CALLBACK(msi_receiver_write_callback), scml2::NEVER_SYNCING);
    scml2::set_read_callback(msi_outstanding_reg_, SCML2_CALLBACK(msi_outstanding_read_callback), scml2::NEVER_SYNCING);
    scml2::set_read_callback(msix_pba_reg_, SCML2_CALLBACK(msix_pba_read_callback), scml2::NEVER_SYNCING);
    
    // Configure MSI-X table register callbacks
    // For indexed arrays, use memory-based callbacks with tags to identify the index
    // The register objects provide structured access, while callbacks sync with internal state
    for (uint8_t i = 0; i < num_vectors_; i++) {
        // Calculate table offset for future callback registration
        // (void)table_offset;  // Suppress unused variable warning
        // uint32_t table_offset = MSIX_TABLE_BASE_OFFSET + (i * MSIX_TABLE_ENTRY_SIZE);
        
        // TODO: Register callbacks for MSI-X table registers
        // For now, table entries are accessed directly via register objects
        // Callbacks can be added later if needed for synchronization
        (void)i;  // Suppress unused variable warning until callbacks are implemented
    }
    
    // For msi_apb_socket, bind adapter to memory for automatic routing
    // Special routing to msi_receiver (offset 0) is handled in msi_b_transport
    // Note: msi_apb_socket is a tlm_base_target_socket, so we use the adapter
    msi_adapter_(csr_memory_);
    
    // Register SC_THREAD for MSI thrower process
    SC_THREAD(msi_thrower_process);
    sensitive << msix_enable << msix_mask << setip;
}

MsiRelayUnit::~MsiRelayUnit() {
}

void MsiRelayUnit::write_msi_receiver(uint32_t data) {
    // Extract interrupt vector index from data[15:0]
    uint8_t vector_index = data & 0xFFFF;
    
    if (vector_index < num_vectors_) {
        set_pba_bit(vector_index);
    }
}

uint32_t MsiRelayUnit::read_msi_outstanding() const {
    return msi_outstanding_;
}

uint32_t MsiRelayUnit::read_msix_pba() const {
    return msix_pba_.to_uint();
}

void MsiRelayUnit::write_msix_table(uint8_t index, uint64_t address, uint32_t data, bool mask) {
    if (index < num_vectors_) {
        msix_table_[index].address = address;
        msix_table_[index].data = data;
        msix_table_[index].mask = mask;
    }
}

void MsiRelayUnit::read_msix_table(uint8_t index, uint64_t& address, uint32_t& data, bool& mask) const {
    if (index < num_vectors_) {
        address = msix_table_[index].address;
        data = msix_table_[index].data;
        mask = msix_table_[index].mask;
    } else {
        address = 0;
        data = 0;
        mask = true;
    }
}

void MsiRelayUnit::set_pba_bit(uint8_t index) {
    if (index < num_vectors_) {
        msix_pba_[index] = 1;
    }
}

void MsiRelayUnit::clear_pba_bit(uint8_t index) {
    if (index < num_vectors_) {
        msix_pba_[index] = 0;
    }
}

bool MsiRelayUnit::is_msi_allowed(uint8_t index) const {
    if (index >= num_vectors_) {
        return false;
    }
    
    // Check if MSI-X is enabled
    if (!msix_enable.read()) {
        return false;
    }
    
    // Check if MSI-X is masked globally
    if (msix_mask.read()) {
        return false;
    }
    
    // Check if this vector is masked
    if (msix_table_[index].mask) {
        return false;
    }
    
    // Check if PBA bit is set
    if (!msix_pba_[index]) {
        return false;
    }
    
    // Check if MSI-X table entry is valid (address != 0)
    if (msix_table_[index].address == 0) {
        return false;
    }
    
    return true;
}

void MsiRelayUnit::send_msi(uint8_t index) {
    if (index >= num_vectors_) {
        return;
    }
    
    const MsixTableEntry& entry = msix_table_[index];
    
    // Create AXI4-Lite write transaction
    tlm::tlm_generic_payload trans;
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    
    uint32_t data = entry.data;
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(&data);
    
    trans.set_command(tlm::TLM_WRITE_COMMAND);
    trans.set_address(entry.address);
    trans.set_data_ptr(data_ptr);
    trans.set_data_length(4);  // 32-bit write
    trans.set_streaming_width(4);
    trans.set_byte_enable_ptr(0);
    trans.set_dmi_allowed(false);
    trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    
    // Increment outstanding count
    msi_outstanding_++;
    
    // SCML initiator socket supports TLM b_transport interface
    // simple_initiator_socket::operator->() provides access to TLM interface
    msi_axi_socket->b_transport(trans, delay);
    
    // Check completion (in real hardware, this would be async)
    if (trans.get_response_status() == tlm::TLM_OK_RESPONSE) {
        // MSI sent successfully - clear PBA bit
        clear_pba_bit(index);
    }
    
    // Decrement outstanding count when transaction completes
    // Note: In a real implementation, this would be done when response arrives
    msi_outstanding_--;
}

void MsiRelayUnit::msi_thrower_process() {
    while (true) {
        wait();  // Wait for sensitivity list events
        
        // Check all vectors for pending MSIs
        for (uint8_t i = 0; i < num_vectors_; i++) {
            if (is_msi_allowed(i)) {
                send_msi(i);
                // Only send one MSI per cycle to avoid overwhelming the bus
                break;
            }
        }
    }
}

// Callback implementations for SCML register writes
// According to VZ_SCMLRef.md Section 2.5.4, write callbacks use:
// bool writeCallback(const DT& data, const DT& byteEnables, sc_core::sc_time&, int tag)
bool MsiRelayUnit::msi_receiver_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // Extract interrupt vector index from data[15:0]
    uint8_t vector_index = data & 0xFFFF;
    if (vector_index < num_vectors_) {
        set_pba_bit(vector_index);
    }
    return true;  // Return true to indicate successful access
}

bool MsiRelayUnit::msix_table_addr_low_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // TODO: Track which register index this corresponds to using tag parameter
    // For now, this callback is not used - table entries accessed directly
    return true;
}

bool MsiRelayUnit::msix_table_addr_high_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // TODO: Track which register index this corresponds to using tag parameter
    // For now, this callback is not used - table entries accessed directly
    return true;
}

bool MsiRelayUnit::msix_table_data_write_callback(const uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // TODO: Track which register index this corresponds to using tag parameter
    // Extract mask bit (bit 0) and update table entry
    // For now, this callback is not used - table entries accessed directly
    return true;
}

// Callback implementations for SCML register reads
// According to VZ_SCMLRef.md Section 2.5.4, read callbacks use:
// bool readCallback(DT& data, const DT& byteEnables, sc_core::sc_time&, int tag)
bool MsiRelayUnit::msi_outstanding_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    data = read_msi_outstanding();
    return true;  // Return true to indicate successful access
}

bool MsiRelayUnit::msix_pba_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    data = read_msix_pba();
    return true;  // Return true to indicate successful access
}

bool MsiRelayUnit::msix_table_addr_low_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // TODO: Track which register index this corresponds to using tag parameter
    // For now, this callback is not used - table entries accessed directly
    return true;
}

bool MsiRelayUnit::msix_table_addr_high_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // TODO: Track which register index this corresponds to using tag parameter
    // For now, this callback is not used - table entries accessed directly
    return true;
}

bool MsiRelayUnit::msix_table_data_read_callback(uint32_t& data, const uint32_t& byteEnables, sc_core::sc_time& t) {
    // TODO: Track which register index this corresponds to using tag parameter
    // For now, this callback is not used - table entries accessed directly
    return true;
}

// Note: csr_b_transport removed - now handled by SCML port adapter and callbacks
// The adapter automatically routes transactions to csr_memory_ and triggers callbacks

tlm::tlm_sync_enum MsiRelayUnit::msi_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint32_t offset = static_cast<uint32_t>(addr);
    
    // APB Port 2 automatically routes to msi_receiver register
    if (trans.get_command() == tlm::TLM_WRITE_COMMAND && offset == 0) {
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        uint32_t data = *data_ptr;
        write_msi_receiver(data);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        // Read from msi_receiver returns 0 or DECERR (spec says may return 0)
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        *data_ptr = 0;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
    
    return tlm::TLM_COMPLETED;
}

} // namespace pcie
} // namespace keraunos

