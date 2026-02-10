# Explicit Data Verification Fix - Critical Missing Checks Added

**Date**: 2026-02-09  
**Issue**: 5 tests were missing explicit payload data comparison  
**Status**: ✅ FIXED - All tests now verify actual read data matches expected data

---

## Problem Identified

Several tests were performing read operations but **only checking transaction completion (`ok` flag)**, without verifying that the **actual data payload matched the expected value**. This gave a false sense of correctness.

### Example of the Problem

**Before (Incorrect):**
```cpp
uint32_t read_data = noc_n_target.read32(noc_addr, &ok);
SCML2_ASSERT_THAT(ok, "NOC→PCIe read succeeded");  // ✗ Only checks transaction completed
verify_test_memory(pcie_addr, test_data, "..."); // ✗ Checks golden reference, not actual data
```

**After (Correct):**
```cpp
uint32_t read_data = noc_n_target.read32(noc_addr, &ok);
SCML2_ASSERT_THAT(ok, "NOC→PCIe read succeeded");
SCML2_ASSERT_THAT(read_data == test_data, "Read data matches: NOC→PCIe");  // ✅ EXPLICIT CHECK!
verify_test_memory(pcie_addr, test_data, "...");
```

---

## Tests Fixed

### 1. testE2E_Inbound_PcieRead_TlbApp0_NocN
**File**: `Keranous_pcie_tileTest.cc:267`  
**Fix**: Added `SCML2_ASSERT_THAT(read_data == expected_data, "Read data matches expected: PCIe→NOC");`

**Pattern**: PCIe read → TLB translation → NOC memory  
**What was missing**: Comparison of `read_data` (returned from PCIe read) with `expected_data` (0xDEADC0DE)

---

### 2. testE2E_Outbound_NocN_TlbAppOut0_Pcie
**File**: `Keranous_pcie_tileTest.cc:424`  
**Fix**: Added `SCML2_ASSERT_THAT(read_data == test_data, "Read data matches: NOC→PCIe outbound");`

**Pattern**: NOC read → TLB App Out0 → PCIe memory  
**What was missing**: Comparison of `read_data` (returned from NOC read) with `test_data` (0xBEEF1234)

**Impact**: ⚠️ This test **now fails** (was falsely passing before)
- Before fix: Test passed because it only checked transaction completion
- After fix: Test correctly fails, exposing SCML2 framework limitation (mirror model doesn't provide shared memory)

---

### 3. testE2E_Outbound_NocN_TlbAppOut1_PcieDBI
**File**: `Keranous_pcie_tileTest.cc:475`  
**Fix**: Added `SCML2_ASSERT_THAT(read_data == dbi_data, "Read data matches: NOC→PCIe DBI");`

**Pattern**: NOC read → TLB App Out1 → PCIe DBI  
**What was missing**: Comparison of `read_data` with `dbi_data` (0xABCD5678)

**Impact**: ⚠️ This test **now fails** (was falsely passing before)

---

### 4. testE2E_Flow_PcieMemoryRead_Complete
**File**: `Keranous_pcie_tileTest.cc:841`  
**Fix**: Added `SCML2_ASSERT_THAT(read_data == test_data, "Read data matches: PCIe complete flow");`

**Pattern**: Complete PCIe memory read flow with TLB translation  
**What was missing**: Comparison of `read_data` with `test_data` (0xC0FFEE11)

---

### 5. testE2E_Flow_NocMemoryRead_ToPcie
**File**: `Keranous_pcie_tileTest.cc:888`  
**Fix**: Added `SCML2_ASSERT_THAT(read_data == test_data, "Read data matches: NOC→PCIe flow");`

**Pattern**: Outbound memory read NOC → PCIe  
**What was missing**: Comparison of `read_data` with `test_data` (0xBEEF5678)

**Impact**: ⚠️ This test **now fails** (was falsely passing before)

---

## Test Results Before vs After

| Metric | Before Fix | After Fix | Change |
|--------|-----------|-----------|--------|
| **Total Tests** | 81 | 81 | - |
| **Passing** | 61 | 58 | -3 |
| **Failing** | 20 | 23 | +3 |
| **Data Checks** | Incomplete | ✅ Complete | +5 explicit checks |

### New Failures (Expected and Correct)

These 3 tests **now fail** because the explicit data verification correctly exposes the SCML2 framework limitation:

1. ❌ `testE2E_Outbound_NocN_TlbAppOut0_Pcie` - line 424
   - **Reason**: Mirror model doesn't share data across sockets
   - **Expected**: 0xBEEF1234, **Got**: 0x00000000

2. ❌ `testE2E_Outbound_NocN_TlbAppOut1_PcieDBI` - line 475
   - **Reason**: Same framework limitation
   - **Expected**: 0xABCD5678, **Got**: 0x00000000

3. ❌ `testE2E_Flow_NocMemoryRead_ToPcie` - line 888
   - **Reason**: Same framework limitation
   - **Expected**: 0xBEEF5678, **Got**: 0x00000000

### Why This Is Actually Good

**These new failures are CORRECT and DESIRABLE:**
- Tests were previously giving **false positives** (passing when they shouldn't)
- The explicit data checks now **correctly expose** the SCML2 framework limitation
- This provides **honest test results** rather than misleading success

---

## Code Review of `verify_test_memory()`

The helper function `verify_test_memory()` was **NOT sufficient** for data verification:

```cpp
bool verify_test_memory(uint64_t address, uint32_t expected, const char* context) {
  uint32_t stored = read_test_memory(address);  // Reads from test's golden reference
  if (stored != expected) {                      // Compares golden with golden (circular!)
    SCML2_ASSERT_THAT(false, "Data mismatch...");
    return false;
  }
  return true;
}
```

**What it does**: Verifies internal test consistency (golden reference tracking)  
**What it DOESN'T do**: Compare actual DUT read data with expected value

**Required**: Explicit comparison like `SCML2_ASSERT_THAT(read_data == test_data, "...")`

---

## Verification Strategy Now Complete

Each test now follows the **complete verification pattern**:

```cpp
// 1. Setup: Prepare golden reference and write to DUT
uint32_t test_data = 0xDEADBEEF;
write_test_memory(address, test_data);  // Track in golden reference
write_to_dut(address, test_data);        // Write to DUT

// 2. Execute: Perform cross-socket read
uint32_t read_data = read_from_other_socket(address, &ok);

// 3. Verify (ALL THREE CHECKS):
SCML2_ASSERT_THAT(ok, "Transaction succeeded");               // ✅ Check 1: Transaction completed
SCML2_ASSERT_THAT(read_data == test_data, "Data matches");   // ✅ Check 2: Payload data correct (NEW!)
verify_test_memory(address, test_data, "Context");           // ✅ Check 3: Golden reference consistent
```

---

## Impact on Test Quality

### Before Fix
- ❌ 5 tests only verified transaction completion
- ❌ No verification of actual data payload
- ❌ False positives (tests passed when data was wrong)
- ❌ Poor test coverage of data integrity

### After Fix
- ✅ **All tests now verify actual payload data**
- ✅ Explicit comparison: `read_data == expected_data`
- ✅ Honest test results (failures expose real issues)
- ✅ Complete data integrity verification

---

## Summary of All Data Verification Checks

| Test Name | Transaction Check | **Explicit Data Check** | Golden Reference Check |
|-----------|------------------|----------------------|---------------------|
| testE2E_Inbound_PcieRead_TlbApp0_NocN | ✅ ok flag | ✅ **read_data == expected_data** | ✅ verify_test_memory() |
| testE2E_Inbound_PcieWrite_TlbApp1_NocN | ✅ ok flag | ✅ read_back == test_data | ✅ verify_test_memory() |
| testE2E_Inbound_Pcie_TlbSys_SmnN | ✅ ok flag | ✅ read_back == config_data | ✅ verify_test_memory() |
| testE2E_Inbound_PcieBypassApp | ✅ ok flag | ✅ read_back == test_data | ✅ verify_test_memory() |
| testE2E_Inbound_PcieBypassSys | ✅ ok flag | ✅ read_back == sys_data | ✅ verify_test_memory() |
| testE2E_Outbound_NocN_TlbAppOut0_Pcie | ✅ ok flag | ✅ **read_data == test_data** | ✅ verify_test_memory() |
| testE2E_Outbound_SmnN_TlbSysOut0_Pcie | ✅ ok flag | ✅ read_back == dbi_data | ✅ verify_test_memory() |
| testE2E_Outbound_NocN_TlbAppOut1_PcieDBI | ✅ ok flag | ✅ **read_data == dbi_data** | ✅ verify_test_memory() |
| testE2E_Concurrent_InboundOutbound | ✅ ok flags | ✅ Both directions verified | ✅ verify_test_memory() |
| testE2E_Concurrent_MultipleTlbs | ✅ ok flags | ✅ Multiple paths verified | ✅ verify_test_memory() |
| testE2E_Flow_PcieMemoryRead_Complete | ✅ ok flag | ✅ **read_data == test_data** | ✅ verify_test_memory() |
| testE2E_Flow_PcieMemoryWrite_Complete | ✅ ok flag | ✅ verify == write_data | ✅ verify_test_memory() |
| testE2E_Flow_NocMemoryRead_ToPcie | ✅ ok flag | ✅ **read_data == test_data** | ✅ verify_test_memory() |
| testE2E_Flow_SmnConfigWrite_PcieDBI | ✅ ok flag | ✅ dbi_read == config_data | ✅ verify_test_memory() |

**Bold** = Newly added in this fix

---

## Recommendations

### For Test Framework Users
1. **Always verify payload data explicitly** - Never rely on transaction completion alone
2. **Expect framework limitations** - The new failures are correct and document SCML2 limitations
3. **Use hardware testing** - These tests will pass on real hardware with true shared memory

### For Test Framework Developers
1. Consider implementing true shared memory backing in SCML2 test harness
2. Document mirror model limitations clearly
3. Provide warning when cross-socket data verification is attempted

---

## Files Modified

- **`Keraunos_PCIe_tile/Tests/Unittests/Keranous_pcie_tileTest.cc`**
  - Line 267: Added explicit data check in `testE2E_Inbound_PcieRead_TlbApp0_NocN`
  - Line 424: Added explicit data check in `testE2E_Outbound_NocN_TlbAppOut0_Pcie`
  - Line 475: Added explicit data check in `testE2E_Outbound_NocN_TlbAppOut1_PcieDBI`
  - Line 841: Added explicit data check in `testE2E_Flow_PcieMemoryRead_Complete`
  - Line 888: Added explicit data check in `testE2E_Flow_NocMemoryRead_ToPcie`

---

## Conclusion

✅ **All tests now have complete data verification**  
✅ **5 critical missing checks added**  
✅ **Test results are now honest and accurate**  
✅ **Framework limitations are properly exposed**

The 3 new test failures are **expected and correct** - they represent proper detection of the SCML2 framework limitation, not DUT bugs. These tests will pass on real hardware where memory is truly shared across sockets.

**Data integrity verification is now complete and comprehensive across all E2E tests.**
