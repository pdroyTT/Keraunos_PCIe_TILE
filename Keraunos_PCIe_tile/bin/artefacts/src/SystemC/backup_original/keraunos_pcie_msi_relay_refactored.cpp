#include "keraunos_pcie_msi_relay_refactored.h"
#include <cstring>
#include <iostream>

namespace keraunos {
namespace pcie {

MsiRelayRefactored::MsiRelayRefactored(uint8_t num_vectors)
    : num_vectors_(num_vectors)
    , msix_table_(num_vectors)
    , msix_pba_(0)
    , msi_outstanding_(0)
    , msix_enable_(false)
    , msix_mask_(false)
    , setip_(0)
    , msi_output_callback_(nullptr)
{
    // Initialize MSI-X table entries
    for (auto& entry : msix_table_) {
        entry.address = 0;
        entry.data = 0;
        entry.mask = true;  // Masked by default
    }
}

// ============================================================================
// Function Interfaces (replace sockets)
// ============================================================================

void MsiRelayRefactored::process_csr_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        process_csr_read(trans, delay);
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        process_csr_write(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void MsiRelayRefactored::process_msi_input(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint32_t offset = static_cast<uint32_t>(addr);
    
    // MSI input automatically routes to msi_receiver register (offset 0)
    if (trans.get_command() == tlm::TLM_WRITE_COMMAND && offset == 0) {
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        uint32_t data = *data_ptr;
        write_msi_receiver(data);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        // Read from msi_receiver returns 0
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        *data_ptr = 0;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

void MsiRelayRefactored::set_msi_output_callback(TransportCallback callback) {
    msi_output_callback_ = callback;
}

// ============================================================================
// Control Signal Interfaces
// ============================================================================

void MsiRelayRefactored::set_msix_enable(bool enable) {
    msix_enable_ = enable;
}

void MsiRelayRefactored::set_msix_mask(bool mask) {
    msix_mask_ = mask;
}

void MsiRelayRefactored::set_interrupt_pending(uint16_t setip_bits) {
    setip_ = setip_bits;
}

// ============================================================================
// Register Access Methods
// ============================================================================

void MsiRelayRefactored::write_msi_receiver(uint32_t data) {
    // Extract interrupt vector index from data[15:0]
    uint8_t vector_index = data & 0xFFFF;
    
    if (vector_index < num_vectors_) {
        set_pba_bit(vector_index);
    }
}

uint32_t MsiRelayRefactored::read_msi_outstanding() const {
    return msi_outstanding_;
}

uint32_t MsiRelayRefactored::read_msix_pba() const {
    return static_cast<uint32_t>(msix_pba_);
}

void MsiRelayRefactored::write_msix_table(uint8_t index, uint64_t address, uint32_t data, bool mask) {
    if (index < num_vectors_) {
        msix_table_[index].address = address;
        msix_table_[index].data = data;
        msix_table_[index].mask = mask;
    }
}

void MsiRelayRefactored::read_msix_table(uint8_t index, uint64_t& address, uint32_t& data, bool& mask) const {
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

// ============================================================================
// Processing Method
// ============================================================================

void MsiRelayRefactored::process_pending_msis() {
    // Check all vectors for pending MSIs (mimics the SC_THREAD behavior)
    for (uint8_t i = 0; i < num_vectors_; i++) {
        if (is_msi_allowed(i)) {
            send_msi(i);
            // Only send one MSI per call to avoid overwhelming the bus
            break;
        }
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void MsiRelayRefactored::set_pba_bit(uint8_t index) {
    if (index < num_vectors_) {
        msix_pba_ |= (1 << index);
    }
}

void MsiRelayRefactored::clear_pba_bit(uint8_t index) {
    if (index < num_vectors_) {
        msix_pba_ &= ~(1 << index);
    }
}

bool MsiRelayRefactored::is_msi_allowed(uint8_t index) const {
    if (index >= num_vectors_) {
        return false;
    }
    
    // Check if MSI-X is enabled
    if (!msix_enable_) {
        return false;
    }
    
    // Check if MSI-X is masked globally
    if (msix_mask_) {
        return false;
    }
    
    // Check if this vector is masked
    if (msix_table_[index].mask) {
        return false;
    }
    
    // Check if PBA bit is set
    if (!(msix_pba_ & (1 << index))) {
        return false;
    }
    
    // Check if MSI-X table entry is valid (address != 0)
    if (msix_table_[index].address == 0) {
        return false;
    }
    
    return true;
}

void MsiRelayRefactored::send_msi(uint8_t index) {
    if (index >= num_vectors_) {
        return;
    }
    
    if (!msi_output_callback_) {
        std::cerr << "MsiRelayRefactored: MSI output callback not set!" << std::endl;
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
    
    // Call output callback (replaces socket->b_transport)
    msi_output_callback_(trans, delay);
    
    // Check completion
    if (trans.get_response_status() == tlm::TLM_OK_RESPONSE) {
        // MSI sent successfully - clear PBA bit
        clear_pba_bit(index);
    }
    
    // Decrement outstanding count when transaction completes
    msi_outstanding_--;
}

// ============================================================================
// CSR Processing Helpers
// ============================================================================

void MsiRelayRefactored::process_csr_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint32_t offset = static_cast<uint32_t>(addr);
    uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
    
    if (offset == MSI_RECEIVER_OFFSET) {
        // Read from msi_receiver returns 0 (write-only)
        *data_ptr = 0;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset == MSI_OUTSTANDING_OFFSET) {
        *data_ptr = read_msi_outstanding();
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset == MSIX_PBA_OFFSET) {
        *data_ptr = read_msix_pba();
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset >= MSIX_TABLE_BASE_OFFSET) {
        // MSI-X table access
        uint32_t table_offset = offset - MSIX_TABLE_BASE_OFFSET;
        uint8_t index = table_offset / MSIX_TABLE_ENTRY_SIZE;
        uint8_t field_offset = table_offset % MSIX_TABLE_ENTRY_SIZE;
        
        if (index < num_vectors_) {
            const MsixTableEntry& entry = msix_table_[index];
            if (field_offset == 0) {
                // Address low
                *data_ptr = static_cast<uint32_t>(entry.address & 0xFFFFFFFF);
            } else if (field_offset == 4) {
                // Address high
                *data_ptr = static_cast<uint32_t>((entry.address >> 32) & 0xFFFFFFFF);
            } else if (field_offset == 8) {
                // Data
                *data_ptr = entry.data;
            } else if (field_offset == 12) {
                // Mask
                *data_ptr = entry.mask ? 1 : 0;
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

void MsiRelayRefactored::process_csr_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint32_t offset = static_cast<uint32_t>(addr);
    uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
    uint32_t data = *data_ptr;
    
    if (offset == MSI_RECEIVER_OFFSET) {
        write_msi_receiver(data);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset == MSI_OUTSTANDING_OFFSET) {
        // Read-only register
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset == MSIX_PBA_OFFSET) {
        // Read-only register
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset >= MSIX_TABLE_BASE_OFFSET) {
        // MSI-X table access
        uint32_t table_offset = offset - MSIX_TABLE_BASE_OFFSET;
        uint8_t index = table_offset / MSIX_TABLE_ENTRY_SIZE;
        uint8_t field_offset = table_offset % MSIX_TABLE_ENTRY_SIZE;
        
        if (index < num_vectors_) {
            MsixTableEntry& entry = msix_table_[index];
            if (field_offset == 0) {
                // Address low
                entry.address = (entry.address & 0xFFFFFFFF00000000ULL) | data;
            } else if (field_offset == 4) {
                // Address high
                entry.address = (entry.address & 0x00000000FFFFFFFFULL) | (static_cast<uint64_t>(data) << 32);
            } else if (field_offset == 8) {
                // Data
                entry.data = data;
            } else if (field_offset == 12) {
                // Mask
                entry.mask = (data & 0x1) != 0;
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

} // namespace pcie
} // namespace keraunos
