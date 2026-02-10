# SCML Compliance Report for Keraunos PCIE Tile Implementation

**Date:** 2025-01-XX  
**Reference Document:** VZ_SCMLRef.md (SystemC Modeling Library Reference Manual X-2025.06-SP1)  
**Implementation:** Keraunos PCIE Tile SystemC/TLM2.0 Models

---

## Executive Summary

This report analyzes the SCML compliance of the Keraunos PCIE Tile SystemC/TLM2.0 implementation against the SCML2 reference manual. The implementation shows **partial compliance** with several areas requiring updates to fully align with SCML2 best practices.

**Overall Compliance Status:** ⚠️ **PARTIAL COMPLIANCE** (60%)

---

## Compliance Analysis

### 1. Module Structure ✅ COMPLIANT

**Status:** ✅ **COMPLIANT**

**Analysis:**
- All modules correctly inherit from `sc_core::sc_module`
- Proper use of `SC_HAS_PROCESS` macro
- Correct namespace usage (`scml2` namespace)

**Evidence:**
```cpp
class TLBSysIn0 : public sc_core::sc_module {
    SC_HAS_PROCESS(TLBSysIn0);
    ...
};
```

**Reference:** VZ_SCMLRef.md Section 8.1 (Modeling Guidelines)

---

### 2. Socket Usage ⚠️ PARTIALLY COMPLIANT

**Status:** ⚠️ **PARTIALLY COMPLIANT**

**Analysis:**

#### ✅ Compliant Aspects:
- Correct use of `scml2::target_socket<32>` for APB configuration interfaces
- Correct use of `scml2::initiator_socket` would be appropriate for initiator ports (though currently using `tlm::tlm_initiator_socket`)

#### ❌ Non-Compliant Aspects:

1. **TLM2.0 Sockets Instead of SCML Sockets:**
   - Current implementation uses `tlm::tlm_target_socket<64>` and `tlm::tlm_initiator_socket<64>` for AXI interfaces
   - SCML recommends `scml2::target_socket` and `scml2::initiator_socket` for LT coding style
   - However, this is acceptable if using pure TLM2.0 style

2. **Manual Transport Method Registration:**
   - Current: `inbound_socket.register_b_transport(this, &TLBSysIn0::b_transport);`
   - SCML approach: Use port adapters to bind memory objects to sockets

**Reference:** VZ_SCMLRef.md Section 3.2.2 (Sockets), Section 8.1.2

**Recommendation:**
- For pure TLM2.0 style: Current approach is acceptable
- For SCML style: Use `scml2::target_socket` with port adapters

---

### 3. Memory Object Usage ❌ NON-COMPLIANT

**Status:** ❌ **NON-COMPLIANT**

**Critical Issue:** Memory objects are not properly bound to sockets using port adapters.

**Current Implementation:**
```cpp
// In header
scml2::memory<uint64_t> tlb_memory_;

// In constructor
tlb_memory_("tlb_memory", 64 * 8);

// Manual TLM transport methods used instead
inbound_socket.register_b_transport(this, &TLBSysIn0::b_transport);
```

**SCML Requirement (from VZ_SCMLRef.md Section 2.2.1):**
> "An scml2::memory can be bound to a target socket using a port_adapter."

**SCML-Compliant Approach:**
```cpp
// Header
scml2::target_socket<32> config_socket;
scml2::tlm2_gp_target_adapter<32> mAdapter;
scml2::memory<uint64_t> tlb_memory_;

// Constructor
TLBSysIn0(sc_core::sc_module_name name)
    : sc_module(name)
    , mAdapter("adapter")
    , tlb_memory_("tlb_memory", 64 * 8)
{
    mAdapter(config_socket);  // Bind adapter to socket
    mAdapter(tlb_memory_);     // Bind adapter to memory
}
```

**Reference:** VZ_SCMLRef.md Section 2.2.1, Section 8.1.4, Section 2.9.3

**Impact:** 
- Memory objects exist but are not integrated with SCML's transaction routing
- Manual TLM transport bypasses SCML's automatic transaction handling
- Missing benefits: automatic DMI handling, callback support, watchpoint support

**Recommendation:** 
- **HIGH PRIORITY:** Implement port adapters for configuration socket + memory binding
- For TLB translation logic (passthrough), manual TLM transport is acceptable

---

### 4. MSI Relay Unit Memory ❌ NON-COMPLIANT

**Status:** ❌ **NON-COMPLIANT**

**Current Implementation:**
```cpp
scml2::memory<uint8_t> csr_memory_("csr_memory", 16 * 1024);

// Manual transport methods
tlm::tlm_sync_enum csr_b_transport(tlm::tlm_generic_payload& trans, ...);
```

**Issue:** 
- Memory object created but not bound to socket via port adapter
- Manual transport methods bypass SCML's memory access mechanisms

**SCML-Compliant Approach:**
```cpp
scml2::target_socket<32> csr_apb_socket;
scml2::tlm2_gp_target_adapter<32> csr_adapter;
scml2::memory<uint8_t> csr_memory_;

// Constructor
csr_adapter("csr_adapter");
csr_adapter(csr_apb_socket);
csr_adapter(csr_memory_);
```

**Reference:** VZ_SCMLRef.md Section 2.2.1, Section 8.1.4

**Recommendation:** 
- **HIGH PRIORITY:** Bind CSR memory to socket using port adapter
- Use SCML register/bitfield objects for structured register access

---

### 5. Register Modeling ❌ NOT IMPLEMENTED

**Status:** ❌ **NOT IMPLEMENTED**

**Current Approach:**
- MSI Relay Unit uses manual register access in `b_transport` methods
- No use of `scml2::reg` or `scml2::bitfield` objects

**SCML Approach:**
```cpp
scml2::memory<uint8_t> csr_memory_;
scml2::reg<uint32_t> msi_receiver_reg;
scml2::reg<uint32_t> msi_outstanding_reg;
scml2::reg<uint32_t> msix_pba_reg;
scml2::reg<uint64_t> msix_table_reg[16];

// In constructor
msi_receiver_reg("msi_receiver", csr_memory_, 0x0000);
msi_outstanding_reg("msi_outstanding", csr_memory_, 0x0004);
msix_pba_reg("msix_pba", csr_memory_, 0x1000);
// etc.
```

**Benefits of SCML Registers:**
- Automatic read/write callbacks
- Watchpoint support for debugging
- Structured access control
- Coverage support

**Reference:** VZ_SCMLRef.md Section 2.5 (reg), Section 2.6 (bitfield)

**Recommendation:**
- **MEDIUM PRIORITY:** Refactor MSI Relay Unit to use `scml2::reg` objects
- Provides better debugging and verification support

---

### 6. Initiator Socket Usage ⚠️ PARTIALLY COMPLIANT

**Status:** ⚠️ **PARTIALLY COMPLIANT**

**Current Implementation:**
```cpp
tlm::tlm_initiator_socket<64> translated_socket;
tlm::tlm_initiator_socket<32> msi_axi_socket;
```

**SCML Recommendation:**
```cpp
scml2::initiator_socket<64> translated_socket;
scml2::initiator_socket<32> msi_axi_socket;
```

**Benefits of `scml2::initiator_socket`:**
- Built-in DMI handler
- Quantum keeper support
- Simplified read/write APIs
- Implements `mappable_if` for router compatibility

**Reference:** VZ_SCMLRef.md Section 3.2.2, Section 8.1.2

**Recommendation:**
- **LOW PRIORITY:** Consider migrating to `scml2::initiator_socket` for better SCML integration
- Current TLM2.0 sockets are functionally correct

---

### 7. Header Files ✅ COMPLIANT

**Status:** ✅ **COMPLIANT**

**Analysis:**
- Correct inclusion of `<scml2.h>` or specific headers
- Proper SystemC and TLM2.0 headers included
- Correct namespace usage

**Evidence:**
```cpp
#include <scml2.h>
#include <systemc>
#include <tlm>
#include <sc_dt.h>
```

**Reference:** VZ_SCMLRef.md Section 1.2

---

### 8. Namespace Usage ✅ COMPLIANT

**Status:** ✅ **COMPLIANT**

**Analysis:**
- Proper use of `scml2` namespace for SCML objects
- Proper use of `sc_core` namespace for SystemC objects
- Proper use of `tlm` namespace for TLM2.0 objects

**Reference:** VZ_SCMLRef.md Section 8.1.1

---

## Summary of Issues

### Critical Issues (Must Fix)

1. **Memory Binding:** `scml2::memory` objects not bound to sockets via port adapters
   - **Files Affected:** All TLB modules, MSI Relay Unit
   - **Impact:** Missing SCML transaction routing, callbacks, DMI support

2. **Port Adapter Missing:** No `scml2::tlm2_gp_target_adapter` usage
   - **Files Affected:** All modules with `scml2::target_socket`
   - **Impact:** Manual TLM transport bypasses SCML infrastructure

### Medium Priority Issues

3. **Register Modeling:** Manual register access instead of `scml2::reg` objects
   - **Files Affected:** `keraunos_pcie_msi_relay.cpp`
   - **Impact:** Missing watchpoint support, structured access

4. **Initiator Sockets:** Using TLM2.0 sockets instead of SCML sockets
   - **Files Affected:** All modules
   - **Impact:** Missing DMI handler, quantum keeper integration

### Low Priority Issues

5. **Quantum Keeper:** Not implemented for initiator sockets
   - **Impact:** Missing temporal decoupling support
   - **Reference:** VZ_SCMLRef.md Section 8.1.3

---

## Compliance Recommendations

### Immediate Actions (High Priority)

1. **Add Port Adapters for Configuration Sockets:**
   ```cpp
   // For each TLB module
   scml2::tlm2_gp_target_adapter<32> config_adapter;
   
   // In constructor
   config_adapter("config_adapter");
   config_adapter(config_socket);
   config_adapter(tlb_memory_);
   ```

2. **Bind MSI Relay CSR Memory:**
   ```cpp
   scml2::tlm2_gp_target_adapter<32> csr_adapter;
   
   // In constructor
   csr_adapter("csr_adapter");
   csr_adapter(csr_apb_socket);
   csr_adapter(csr_memory_);
   ```

### Medium-Term Actions

3. **Refactor MSI Relay Registers:**
   - Replace manual register access with `scml2::reg` objects
   - Use `scml2::bitfield` for individual register fields

4. **Consider SCML Initiator Sockets:**
   - Evaluate migration to `scml2::initiator_socket`
   - Add quantum keeper if temporal decoupling needed

### Long-Term Enhancements

5. **Add Callbacks:**
   - Implement `scml2::memory_callback_base` for custom behavior
   - Use callbacks for TLB translation logic

6. **Add Coverage Support:**
   - Use SCML coverage APIs for verification
   - Implement coverage points for TLB entries

---

## Compliance Scorecard

| Category | Status | Score | Notes |
|----------|--------|-------|-------|
| Module Structure | ✅ Compliant | 100% | Correct inheritance and macros |
| Socket Usage | ⚠️ Partial | 60% | SCML sockets not used for AXI |
| Memory Binding | ❌ Non-Compliant | 0% | Missing port adapters |
| Register Modeling | ❌ Not Implemented | 0% | Manual access instead of reg objects |
| Header Files | ✅ Compliant | 100% | Correct includes |
| Namespace Usage | ✅ Compliant | 100% | Proper namespaces |
| **Overall** | **⚠️ Partial** | **60%** | **Core structure good, binding missing** |

---

## Conclusion

The Keraunos PCIE Tile implementation demonstrates a **solid understanding of SystemC/TLM2.0** and follows many SCML conventions. However, it **does not fully leverage SCML's infrastructure** for memory and register modeling.

**Key Strengths:**
- ✅ Correct module structure
- ✅ Proper use of SCML namespaces
- ✅ Functional TLM2.0 implementation

**Key Weaknesses:**
- ❌ Missing port adapters for memory binding
- ❌ Manual register access instead of SCML reg objects
- ❌ Not using SCML's automatic transaction routing

**Recommendation:** 
To achieve **full SCML compliance**, implement port adapters for memory binding and consider migrating to SCML register objects. The current implementation is **functionally correct** but misses SCML's advanced features for debugging, verification, and tool integration.

---

## References

1. VZ_SCMLRef.md - SystemC Modeling Library Reference Manual X-2025.06-SP1
2. Section 2.2.1 - Transaction Routing From Socket to Memories
3. Section 2.5 - reg (Register Objects)
4. Section 2.9.3 - tlm2_gp_target_adapter
5. Section 8.1 - Modeling Guidelines

---

**Report End**

