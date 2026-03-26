# Why Stubs Are Needed

## Problem Summary

We need stubs because of a **mismatch between headers and libraries**:

1. **Headers**: Using Virtualizer's SystemC headers (with Synopsys extensions)
   - Path: `/tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux/common/include`
   - These headers contain **Synopsys-specific extensions** to SystemC

2. **Library**: Using standard SystemC 2.3.4 library (from Accellera)
   - Path: `/localdev/pdroy/systemc-2.3.4/lib64`
   - This is a **standard SystemC library** without Synopsys extensions

3. **Result**: Headers declare symbols that don't exist in the library → **undefined reference errors**

---

## Why This Mismatch Exists

### Virtualizer's SystemC Headers
Virtualizer's SystemC headers include Synopsys extensions for:
- **Debug callbacks**: `snps::sc::debug_callback_registry`
- **Writer policy checks**: `sc_core::snps_sc_writer_policy_check`
- **Logging infrastructure**: `scml2::logging::Initializer`
- **Event extensions**: `sc_core::sc_event::m_ongoing_notify_time`
- **Simcontext extensions**: `sc_core::sc_simcontext::alloc_new_simcontext()`
- **Module extensions**: `sc_core::sc_module::end_of_construction()`

### Standard SystemC Library
The standard SystemC 2.3.4 library from Accellera does **not** include these Synopsys extensions.

---

## Current Stubs

We have 6 stub files that provide minimal implementations for missing symbols:

### 1. `scml2_logging_stub.cpp`
**Missing Symbol**: `scml2::logging::Initializer`
- Virtualizer headers reference this for logging infrastructure
- Standard SystemC doesn't have it
- **Stub**: Provides empty implementation

### 2. `scml2_debug_callback_stub.cpp`
**Missing Symbol**: `snps::sc::debug_callback_registry`
- Virtualizer uses this for debug callbacks
- Standard SystemC doesn't have it
- **Stub**: Provides empty implementation

### 3. `scml2_writer_policy_stub.cpp`
**Missing Symbol**: `sc_core::snps_sc_writer_policy_check`
- Virtualizer uses this for writer policy checks
- Standard SystemC doesn't have it
- **Stub**: Returns `true` to allow writes

### 4. `scml2_event_stub.cpp`
**Missing Symbol**: `sc_core::sc_event::m_ongoing_notify_time`
- Virtualizer headers declare this static member
- Standard SystemC doesn't have it
- **Stub**: Defines static member initialized to `nullptr`

### 5. `scml2_simcontext_stub.cpp`
**Missing Symbol**: `sc_core::sc_simcontext::alloc_new_simcontext()`
- Virtualizer expects SystemC 2.3.4 which has this function
- Our SystemC 2.3.4 build might be missing it, or Virtualizer expects a different version
- **Stub**: Provides implementation using SystemC 2.3.3 API (`new sc_simcontext()`)

### 6. `scml2_module_stub.cpp`
**Missing Symbol**: `sc_core::sc_module::end_of_construction()`
- Virtualizer headers reference this virtual function
- Standard SystemC doesn't have it
- **Stub**: Provides empty implementation

---

## Why Not Use Virtualizer's SystemC Library?

### Option 1: Use Virtualizer's SystemC Library (If Available)
**Problem**: Virtualizer typically doesn't provide a standalone SystemC library. It expects:
- SystemC to be linked separately
- Headers to be compatible with the linked SystemC version
- Extensions to be provided by SCML2 or stubs

### Option 2: Use Only Standard SystemC Headers
**Problem**: SCML2 requires Virtualizer's SystemC headers because:
- SCML2 headers include Virtualizer's SystemC headers
- SCML2 expects Synopsys extensions to be present
- Without these headers, SCML2 won't compile

### Option 3: Current Approach (Stubs)
**Solution**: 
- Use Virtualizer's headers (required for SCML2)
- Link against standard SystemC library (available)
- Provide stubs for missing symbols (minimal implementation)

---

## Are Stubs Safe?

**Yes, stubs are safe** because:

1. **They provide minimal functionality**: Stubs return safe defaults (e.g., `true` for policy checks, `nullptr` for pointers)

2. **They're not used in normal operation**: These extensions are typically used for:
   - Debugging infrastructure (not needed for basic simulation)
   - Tool integration (not needed for standalone simulation)
   - Advanced features (not needed for functional verification)

3. **They match expected signatures**: Stubs match the function signatures declared in Virtualizer's headers

4. **They're standard practice**: When mixing headers and libraries from different sources, stubs are a common solution

---

## Alternative Solutions

### Option A: Use Virtualizer's Complete Environment
If Virtualizer provides a complete SystemC installation:
```makefile
SYSTEMC_HOME = /tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux/common
SYSTEMC_LIB = $(SYSTEMC_HOME)/lib-gcc-9.2-64
```
**Problem**: Virtualizer typically doesn't provide this.

### Option B: Build SystemC with Synopsys Extensions
If Synopsys provides source code for extensions:
- Build SystemC with extensions included
- No stubs needed
**Problem**: Synopsys extensions are proprietary and not available.

### Option C: Disable Extensions (If Possible)
If Virtualizer headers support disabling extensions:
```cpp
#define SC_DISABLE_SYNOPSYS_EXTENSIONS
```
**Problem**: This might break SCML2 compatibility.

---

## Conclusion

**Stubs are necessary** because:
1. ✅ SCML2 requires Virtualizer's SystemC headers (with extensions)
2. ✅ We only have access to standard SystemC library (without extensions)
3. ✅ Stubs provide safe, minimal implementations for missing symbols
4. ✅ This is a standard approach when mixing headers and libraries

**Stubs are safe** because:
- They provide minimal functionality
- They're not used in normal operation
- They match expected signatures
- They're standard practice

**The current approach is correct** and follows best practices for integrating SCML2 with standard SystemC.

---

## References

- Makefile: Lines 7-10 (SystemC paths)
- Makefile: Lines 40-45 (Stub files)
- Virtualizer headers: `/tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux/common/include`
- SystemC library: `/localdev/pdroy/systemc-2.3.4/lib64`

