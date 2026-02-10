#include "keraunos_pcie_msi_relay.h"
#include <cstring>
#include <iostream>

namespace keraunos {
namespace pcie {

MsiRelayUnit::MsiRelayUnit(uint8_t num_vectors)
    : num_vectors_(num_vectors)
    , msix_table_(num_vectors)
    , msix_pba_(0)
    , msi_outstanding_(0)
    , msix_enable_(false)
    , msix_mask_(false)
    , setip_(0)
    , msi_output_callback_(nullptr)
{
    for (auto& entry : msix_table_) {
        entry.address = 0;
        entry.data = 0;
        entry.mask = true;
    }
}

void MsiRelayUnit::process_csr_access(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        process_csr_read(trans, delay);
    } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
        process_csr_write(trans, delay);
    } else {
        trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
    }
}

void MsiRelayUnit::process_msi_input(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    
    if (trans.get_command() == tlm::TLM_WRITE_COMMAND && offset == 0) {
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        write_msi_receiver(*data_ptr);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (trans.get_command() == tlm::TLM_READ_COMMAND) {
        uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
        *data_ptr = 0;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}

void MsiRelayUnit::set_msi_output_callback(TransportCallback callback) {
    msi_output_callback_ = callback;
}

void MsiRelayUnit::set_msix_enable(bool enable) { msix_enable_ = enable; }
void MsiRelayUnit::set_msix_mask(bool mask) { msix_mask_ = mask; }
void MsiRelayUnit::set_interrupt_pending(uint16_t setip_bits) { setip_ = setip_bits; }

void MsiRelayUnit::write_msi_receiver(uint32_t data) {
    uint8_t vector_index = data & 0xFFFF;
    if (vector_index < num_vectors_) {
        set_pba_bit(vector_index);
    }
}

uint32_t MsiRelayUnit::read_msi_outstanding() const {
    return msi_outstanding_;
}

uint32_t MsiRelayUnit::read_msix_pba() const {
    return static_cast<uint32_t>(msix_pba_);
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

void MsiRelayUnit::process_pending_msis() {
    for (uint8_t i = 0; i < num_vectors_; i++) {
        if (is_msi_allowed(i)) {
            send_msi(i);
            break;
        }
    }
}

void MsiRelayUnit::set_pba_bit(uint8_t index) {
    if (index < num_vectors_) {
        msix_pba_ |= (1 << index);
    }
}

void MsiRelayUnit::clear_pba_bit(uint8_t index) {
    if (index < num_vectors_) {
        msix_pba_ &= ~(1 << index);
    }
}

bool MsiRelayUnit::is_msi_allowed(uint8_t index) const {
    if (index >= num_vectors_) return false;
    if (!msix_enable_) return false;
    if (msix_mask_) return false;
    if (msix_table_[index].mask) return false;
    if (!(msix_pba_ & (1 << index))) return false;
    if (msix_table_[index].address == 0) return false;
    return true;
}

void MsiRelayUnit::send_msi(uint8_t index) {
    if (index >= num_vectors_ || !msi_output_callback_) return;
    
    const MsixTableEntry& entry = msix_table_[index];
    tlm::tlm_generic_payload trans;
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    uint32_t data = entry.data;
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(&data);
    
    trans.set_command(tlm::TLM_WRITE_COMMAND);
    trans.set_address(entry.address);
    trans.set_data_ptr(data_ptr);
    trans.set_data_length(4);
    trans.set_streaming_width(4);
    trans.set_byte_enable_ptr(0);
    trans.set_dmi_allowed(false);
    trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    
    msi_outstanding_++;
    msi_output_callback_(trans, delay);
    
    if (trans.get_response_status() == tlm::TLM_OK_RESPONSE) {
        clear_pba_bit(index);
    }
    msi_outstanding_--;
}

void MsiRelayUnit::process_csr_read(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
    
    if (offset == MSI_RECEIVER_OFFSET) {
        *data_ptr = 0;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset == MSI_OUTSTANDING_OFFSET) {
        *data_ptr = read_msi_outstanding();
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset == MSIX_PBA_OFFSET) {
        *data_ptr = read_msix_pba();
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset >= MSIX_TABLE_BASE_OFFSET) {
        uint32_t table_offset = offset - MSIX_TABLE_BASE_OFFSET;
        uint8_t index = table_offset / MSIX_TABLE_ENTRY_SIZE;
        uint8_t field_offset = table_offset % MSIX_TABLE_ENTRY_SIZE;
        
        if (index < num_vectors_) {
            const MsixTableEntry& entry = msix_table_[index];
            if (field_offset == 0) {
                *data_ptr = static_cast<uint32_t>(entry.address & 0xFFFFFFFF);
            } else if (field_offset == 4) {
                *data_ptr = static_cast<uint32_t>((entry.address >> 32) & 0xFFFFFFFF);
            } else if (field_offset == 8) {
                *data_ptr = entry.data;
            } else if (field_offset == 12) {
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

void MsiRelayUnit::process_csr_write(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint32_t offset = static_cast<uint32_t>(trans.get_address());
    uint32_t* data_ptr = reinterpret_cast<uint32_t*>(trans.get_data_ptr());
    uint32_t data = *data_ptr;
    
    if (offset == MSI_RECEIVER_OFFSET) {
        write_msi_receiver(data);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    } else if (offset >= MSIX_TABLE_BASE_OFFSET) {
        uint32_t table_offset = offset - MSIX_TABLE_BASE_OFFSET;
        uint8_t index = table_offset / MSIX_TABLE_ENTRY_SIZE;
        uint8_t field_offset = table_offset % MSIX_TABLE_ENTRY_SIZE;
        
        if (index < num_vectors_) {
            MsixTableEntry& entry = msix_table_[index];
            if (field_offset == 0) {
                entry.address = (entry.address & 0xFFFFFFFF00000000ULL) | data;
            } else if (field_offset == 4) {
                entry.address = (entry.address & 0x00000000FFFFFFFFULL) | (static_cast<uint64_t>(data) << 32);
            } else if (field_offset == 8) {
                entry.data = data;
            } else if (field_offset == 12) {
                entry.mask = (data & 0x1) != 0;
            }
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

} // namespace pcie
} // namespace keraunos
