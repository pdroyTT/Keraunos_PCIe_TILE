# SCML2 Shared Memory Solution for Cross-Socket Testing

## Problem Statement

Current test failures are caused by SCML2 mirror model sockets not sharing memory. When tests write data to PCIe socket and read from NOC socket, they get 0x00000000 because each socket has isolated memory.

## Solution: Use `scml2::memory` with Multiple Adapters

According to SCML2 Reference Manual (PAVP_SCMLRef.md, page 26):

> "Multiple target sockets can be bound, each with its own port_adapter, to the same memory. All target sockets will see the memory at the same address."

This is **exactly** what we need for cross-socket data verification!

## Implementation Approach

### 1. Create Shared Memory Module

Create a new test harness with shared memory that all sockets can access:

```cpp
// File: Keranous_pcie_tileTestHarness_SharedMem.h

#pragma once
#include <scml2_testing.h>
#include <scml2/memory.h>
#include <scml2/tlm2_gp_target_adapter.h>
#include <Keranous_pcie_tileMirror.h>
#include <Keranous_pcie_tileCovermodel.h>

class Keranous_pcie_tileTestHarness_SharedMem 
    : public scml2::testing::test_group<
          ::keraunos::pcie::KeraunosPcieTile, 
          Keranous_pcie_tileMirror, 
          scml2::testing::fast_build_covergroup<Keranous_pcie_tileCovermodel>> {

protected:
    // Shared memory - 16GB address space
    scml2::memory<unsigned int>* shared_memory;
    
    // Target adapters for each socket to access same memory
    scml2::tlm2_gp_target_adapter<32>* pcie_adapter;
    scml2::tlm2_gp_target_adapter<32>* noc_adapter;
    scml2::tlm2_gp_target_adapter<32>* smn_adapter;
    
    // Socket proxies (same as before)
    scml2::testing::target_socket_proxy<64> noc_n_target;
    scml2::testing::target_socket_proxy<64> pcie_controller_target;
    scml2::testing::target_socket_proxy<64> smn_n_target;
    // ... other sockets ...
    
    Keranous_pcie_tileTestHarness_SharedMem() 
        : scml2::testing::test_group<...>() {
        // Create 4GB shared memory (0x100000000 words = 16GB)
        shared_memory = new scml2::memory<unsigned int>("shared_memory", 0x100000000);
        
        // Initialize to known pattern (optional)
        shared_memory->initialize(0xDEADC0DE);
        
        // Create adapters
        pcie_adapter = new scml2::tlm2_gp_target_adapter<32>("pcie_adapter");
        noc_adapter = new scml2::tlm2_gp_target_adapter<32>("noc_adapter");
        smn_adapter = new scml2::tlm2_gp_target_adapter<32>("smn_adapter");
        
        // Bind all adapters to SAME shared memory
        (*pcie_adapter)(*shared_memory);
        (*noc_adapter)(*shared_memory);
        (*smn_adapter)(*shared_memory);
    }
    
    virtual ~Keranous_pcie_tileTestHarness_SharedMem() {
        delete pcie_adapter;
        delete noc_adapter;
        delete smn_adapter;
        delete shared_memory;
    }
    
    virtual void do_initialize_test_group() {
        // Connect DUT initiators to shared memory via adapters
        this->modelUnderTest->pcie_controller_initiator(*pcie_adapter);
        this->modelUnderTest->noc_n_initiator(*noc_adapter);
        this->modelUnderTest->smn_n_initiator(*smn_adapter);
        
        // Connect test proxies to shared memory
        pcie_controller_target.set_target(pcie_adapter);
        noc_n_target.set_target(noc_adapter);
        smn_n_target.set_target(smn_adapter);
        
        // DUT targets connect to mirror model (for register access)
        this->modelUnderTest->pcie_controller_target(this->mirrorModel->pcie_controller_target);
        this->modelUnderTest->noc_n_target(this->mirrorModel->noc_n_target);
        this->modelUnderTest->smn_n_target(this->mirrorModel->smn_n_target);
        
        // ... rest of signal bindings ...
    }
};
```

### 2. Key Benefits

✅ **Shared Memory**: All sockets see same memory at same address  
✅ **Data Persistence**: Write via PCIe, read via NOC - data preserved  
✅ **Address Translation Testing**: TLB translation can be fully verified  
✅ **No DUT Changes**: Solution is test-harness only  
✅ **SCML2 Native**: Uses built-in SCML2 features, not workarounds

### 3. Memory Layout Strategy

```
Address Range         | Purpose
----------------------|------------------------------------------
0x00000000-0x7FFFFFFF | NOC memory space (2GB)
0x80000000-0xBFFFFFFF | PCIe memory space (1GB) 
0xC0000000-0xFFFFFFFF | SMN/Config space (1GB)
0x100000000+          | Extended address space
```

### 4. Update Test Cases

Tests can now verify actual data flow:

```cpp
void testE2E_Inbound_PcieRead_TlbApp0_NocN() {
    // Configure TLB
    configure_tlb_entry_via_smn(0x18210000, 0, 0x80000000, 0x123);
    enable_system();
    
    // Write data to NOC memory via shared memory
    uint32_t expected_data = 0xDEADBEEF;
    bool ok = noc_n_target.write32(0x80001000, expected_data);
    SCML2_ASSERT_THAT(ok, "NOC write should succeed");
    
    // Read via PCIe (should route through TLB to NOC)
    uint32_t actual_data = pcie_controller_target.read32(0x0000000001001000, &ok);
    SCML2_ASSERT_THAT(ok, "PCIe read should succeed");
    
    // VERIFY DATA INTEGRITY - THIS NOW WORKS!
    verify_data_match(expected_data, actual_data, "Cross-socket data flow");
}
```

### 5. Implementation Steps

1. **Create new test harness file** with shared memory
2. **Update test file** to use new harness:
   ```cpp
   // In Keranous_pcie_tileTest.cc
   #include <Keranous_pcie_tileTestHarness_SharedMem.h>
   
   class Keranous_pcie_tileTest : public Keranous_pcie_tileTestHarness_SharedMem {
       // ... rest stays same ...
   };
   ```
3. **Rebuild tests**
4. **Restore original data verification** in retrofitted tests
5. **Run full regression**

### 6. Expected Results

**Before (with mirror model)**: 64/81 passing (79.0%)  
**After (with shared memory)**: **75+/81 passing (93%+)**

All 11 cross-socket tests that we had to modify should now pass with full data verification!

### 7. Alternative: Memory Regions

For more control, use `scml2::memory_alias` to map different regions:

```cpp
// Create 16GB memory
shared_memory = new scml2::memory<unsigned int>("shared_memory", 0x400000000);

// Create aliased regions
noc_region = new scml2::memory_alias<unsigned int>(
    "noc_region", *shared_memory, 0x20000000, 0x20000000);  // 2GB @ 0x80000000

pcie_region = new scml2::memory_alias<unsigned int>(
    "pcie_region", *shared_memory, 0x30000000, 0x10000000); // 1GB @ 0xC0000000

// Bind regions to adapters
(*noc_adapter)(*noc_region);
(*pcie_adapter)(*pcie_region);
```

## References

- **SCML2 Reference Manual**: `/localdev/pdroy/keraunos_pcie_workspace/doc/PAVP_SCMLRef.md`
- **Section 2.3**: `scml2::memory` class (page 35+)
- **Section 2.4**: `scml2::memory_alias` (page 40+)
- **Section 3.5**: `tlm2_gp_target_adapter` (page 68+)

## Recommendation

**Implement this solution** to enable true end-to-end data flow verification. This is the proper way to test cross-socket transactions in SCML2 and will validate the original retrofit goal.

**Estimated Effort**: 4-6 hours (harness creation + test updates + verification)

**Risk**: Low - uses standard SCML2 features, no DUT changes needed
