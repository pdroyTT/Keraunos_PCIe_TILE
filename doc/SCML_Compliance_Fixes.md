# SCML Compliance Fixes - Implementation Guide

This document provides specific code fixes to achieve SCML compliance for the Keraunos PCIE Tile implementation.

---

## Fix 1: Add Port Adapter for TLB Configuration Socket

### File: `include/keraunos_pcie_inbound_tlb.h`

**Current Code:**
```cpp
class TLBSysIn0 : public sc_core::sc_module {
public:
    scml2::target_socket<32> config_socket;
    ...
protected:
    scml2::memory<uint64_t> tlb_memory_;
    ...
};
```

**Fixed Code:**
```cpp
#include <scml2/tlm2_gp_target_adapter.h>

class TLBSysIn0 : public sc_core::sc_module {
public:
    scml2::target_socket<32> config_socket;
    ...
protected:
    scml2::tlm2_gp_target_adapter<32> config_adapter_;
    scml2::memory<uint64_t> tlb_memory_;
    ...
};
```

### File: `src/keraunos_pcie_inbound_tlb.cpp`

**Current Code:**
```cpp
TLBSysIn0::TLBSysIn0(sc_core::sc_module_name name)
    : sc_module(name)
    , entries_(64)
    , tlb_memory_("tlb_memory", 64 * 8)
{
    // Initialize TLB entries
    ...
    
    // Bind TLM sockets
    inbound_socket.register_b_transport(this, &TLBSysIn0::b_transport);
    ...
}
```

**Fixed Code:**
```cpp
TLBSysIn0::TLBSysIn0(sc_core::sc_module_name name)
    : sc_module(name)
    , config_adapter_("config_adapter")
    , entries_(64)
    , tlb_memory_("tlb_memory", 64 * 8)
{
    // Initialize TLB entries
    for (auto& entry : entries_) {
        entry.valid = false;
        entry.addr = 0;
        entry.attr = 0;
    }
    
    // Bind SCML adapter to socket and memory
    config_adapter_(config_socket);
    config_adapter_(tlb_memory_);
    
    // Bind TLM sockets for translation logic (passthrough)
    inbound_socket.register_b_transport(this, &TLBSysIn0::b_transport);
    inbound_socket.register_transport_dbg(this, &TLBSysIn0::transport_dbg);
    inbound_socket.register_get_direct_mem_ptr(this, &TLBSysIn0::get_direct_mem_ptr);
}
```

**Apply Same Fix To:**
- `TLBAppIn0` class
- `TLBAppIn1` class
- `TLBSysOut0` class
- `TLBAppOut0` class
- `TLBAppOut1` class

---

## Fix 2: Add Port Adapter for MSI Relay CSR Socket

### File: `include/keraunos_pcie_msi_relay.h`

**Current Code:**
```cpp
class MsiRelayUnit : public sc_core::sc_module {
public:
    scml2::target_socket<32> csr_apb_socket;
    scml2::target_socket<32> msi_apb_socket;
    ...
protected:
    scml2::memory<uint8_t> csr_memory_;
    ...
};
```

**Fixed Code:**
```cpp
#include <scml2/tlm2_gp_target_adapter.h>

class MsiRelayUnit : public sc_core::sc_module {
public:
    scml2::target_socket<32> csr_apb_socket;
    scml2::target_socket<32> msi_apb_socket;
    ...
protected:
    scml2::tlm2_gp_target_adapter<32> csr_adapter_;
    scml2::tlm2_gp_target_adapter<32> msi_adapter_;
    scml2::memory<uint8_t> csr_memory_;
    ...
};
```

### File: `src/keraunos_pcie_msi_relay.cpp`

**Current Code:**
```cpp
MsiRelayUnit::MsiRelayUnit(sc_core::sc_module_name name, uint8_t num_vectors)
    : sc_module(name)
    , num_vectors_(num_vectors)
    , msix_table_(num_vectors)
    , msix_pba_(0)
    , msi_outstanding_(0)
    , csr_memory_("csr_memory", 16 * 1024)
{
    // Initialize MSI-X table entries
    ...
    
    // Bind TLM sockets
    csr_apb_socket.register_b_transport(this, &MsiRelayUnit::csr_b_transport);
    msi_apb_socket.register_b_transport(this, &MsiRelayUnit::msi_b_transport);
    ...
}
```

**Fixed Code:**
```cpp
MsiRelayUnit::MsiRelayUnit(sc_core::sc_module_name name, uint8_t num_vectors)
    : sc_module(name)
    , csr_adapter_("csr_adapter")
    , msi_adapter_("msi_adapter")
    , num_vectors_(num_vectors)
    , msix_table_(num_vectors)
    , msix_pba_(0)
    , msi_outstanding_(0)
    , csr_memory_("csr_memory", 16 * 1024)
{
    // Initialize MSI-X table entries
    for (auto& entry : msix_table_) {
        entry.address = 0;
        entry.data = 0;
        entry.mask = true;
    }
    
    // Bind SCML adapters to sockets and memory
    csr_adapter_(csr_apb_socket);
    csr_adapter_(csr_memory_);
    
    // MSI adapter for msi_receiver register (special handling)
    msi_adapter_(msi_apb_socket);
    // Note: msi_apb_socket routes to msi_receiver, may need custom callback
    
    // Register SC_THREAD for MSI thrower process
    SC_THREAD(msi_thrower_process);
    sensitive << msix_enable << msix_mask << setip;
}
```

**Note:** For `msi_apb_socket`, you may need a custom callback since it routes specifically to `msi_receiver` register. Consider:

```cpp
// Option 1: Use callback on memory for msi_receiver offset
csr_memory_.set_write_callback(MSI_RECEIVER_OFFSET, 
    scml2::memory_callback_base::create(this, &MsiRelayUnit::msi_receiver_write_callback));

// Option 2: Keep manual transport for msi_apb_socket (acceptable for special routing)
```

---

## Fix 3: Use SCML Register Objects for MSI Relay (Optional Enhancement)

### File: `include/keraunos_pcie_msi_relay.h`

**Enhanced Code:**
```cpp
#include <scml2/reg.h>
#include <scml2/bitfield.h>

class MsiRelayUnit : public sc_core::sc_module {
public:
    ...
protected:
    scml2::memory<uint8_t> csr_memory_;
    
    // SCML register objects
    scml2::reg<uint32_t> msi_receiver_reg_;
    scml2::reg<uint32_t> msi_outstanding_reg_;
    scml2::reg<uint32_t> msix_pba_reg_;
    scml2::reg<uint64_t> msix_table_addr_reg_[16];
    scml2::reg<uint32_t> msix_table_data_reg_[16];
    scml2::bitfield msix_table_mask_bit_[16];
    
    // Callbacks for register access
    void msi_receiver_write_callback(uint32_t old_value, uint32_t new_value);
    uint32_t msi_outstanding_read_callback(uint32_t value);
    uint32_t msix_pba_read_callback(uint32_t value);
    ...
};
```

### File: `src/keraunos_pcie_msi_relay.cpp`

**Enhanced Constructor:**
```cpp
MsiRelayUnit::MsiRelayUnit(sc_core::sc_module_name name, uint8_t num_vectors)
    : sc_module(name)
    , csr_adapter_("csr_adapter")
    , num_vectors_(num_vectors)
    , csr_memory_("csr_memory", 16 * 1024)
    , msi_receiver_reg_("msi_receiver", csr_memory_, MSI_RECEIVER_OFFSET)
    , msi_outstanding_reg_("msi_outstanding", csr_memory_, MSI_OUTSTANDING_OFFSET)
    , msix_pba_reg_("msix_pba", csr_memory_, MSIX_PBA_OFFSET)
{
    // Bind adapter
    csr_adapter_(csr_apb_socket);
    csr_adapter_(csr_memory_);
    
    // Configure register callbacks
    msi_receiver_reg_.set_write_callback(
        scml2::reg_callback_base::create(this, &MsiRelayUnit::msi_receiver_write_callback));
    
    msi_outstanding_reg_.set_read_callback(
        scml2::reg_callback_base::create(this, &MsiRelayUnit::msi_outstanding_read_callback));
    
    msix_pba_reg_.set_read_callback(
        scml2::reg_callback_base::create(this, &MsiRelayUnit::msix_pba_read_callback));
    
    // Initialize MSI-X table registers
    for (uint8_t i = 0; i < num_vectors_; i++) {
        uint32_t table_offset = MSIX_TABLE_BASE_OFFSET + (i * MSIX_TABLE_ENTRY_SIZE);
        msix_table_addr_reg_[i] = scml2::reg<uint64_t>("msix_table_addr", csr_memory_, table_offset);
        msix_table_data_reg_[i] = scml2::reg<uint32_t>("msix_table_data", csr_memory_, table_offset + 8);
        msix_table_mask_bit_[i] = scml2::bitfield("msix_table_mask", msix_table_data_reg_[i], 0, 0);
    }
    
    // Initialize MSI-X table entries
    for (auto& entry : msix_table_) {
        entry.address = 0;
        entry.data = 0;
        entry.mask = true;
    }
    
    SC_THREAD(msi_thrower_process);
    sensitive << msix_enable << msix_mask << setip;
}

void MsiRelayUnit::msi_receiver_write_callback(uint32_t old_value, uint32_t new_value) {
    // Extract interrupt vector index from data[15:0]
    uint8_t vector_index = new_value & 0xFFFF;
    if (vector_index < num_vectors_) {
        set_pba_bit(vector_index);
    }
}

uint32_t MsiRelayUnit::msi_outstanding_read_callback(uint32_t value) {
    return msi_outstanding_;
}

uint32_t MsiRelayUnit::msix_pba_read_callback(uint32_t value) {
    return msix_pba_.to_uint();
}
```

**Benefits:**
- Automatic transaction handling
- Watchpoint support for debugging
- Structured register access
- Coverage support

---

## Fix 4: Consider SCML Initiator Sockets (Optional)

### File: `include/keraunos_pcie_inbound_tlb.h`

**Current:**
```cpp
tlm::tlm_initiator_socket<64> translated_socket;
```

**Optional Enhancement:**
```cpp
scml2::initiator_socket<64> translated_socket;
```

**Benefits:**
- Built-in DMI handler
- Quantum keeper support
- Simplified read/write APIs

**Note:** This is optional - TLM2.0 sockets are functionally correct.

---

## Summary of Required Changes

### Critical (Must Fix for SCML Compliance)

1. ✅ Add `scml2::tlm2_gp_target_adapter` for all configuration sockets
2. ✅ Bind adapters to sockets and memory objects
3. ✅ Update constructors to initialize adapters

### Recommended (For Better SCML Integration)

4. ⚠️ Use `scml2::reg` objects for MSI Relay registers
5. ⚠️ Consider `scml2::initiator_socket` for initiator ports
6. ⚠️ Add quantum keeper for temporal decoupling (if needed)

---

## Testing After Fixes

After implementing these fixes, verify:

1. **Configuration Access:** TLB configuration registers accessible via APB socket
2. **MSI Relay CSR:** CSR registers accessible via APB socket
3. **Memory Binding:** Transactions automatically routed to memory objects
4. **Callbacks:** Register callbacks triggered on access
5. **DMI Support:** Direct memory interface works (if enabled)

---

**Document End**

