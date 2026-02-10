# Test Verification Requirements - PCIe Tile E2E Tests

## Document Purpose

This document defines the verification requirements and standards for all E2E (End-to-End) tests in the Keraunos PCIe Tile test suite.

## Background

### Previous Test Approach (Connectivity-Only)

Original E2E tests only verified architectural connectivity:
- ✅ Transaction completes without error = **PASS**
- ❌ No verification of data integrity
- ❌ No verification of address translation
- ❌ No payload comparison

**Example of Old Test**:
```cpp
void testE2E_Inbound_PcieRead_TlbApp0_NocN() {
    uint64_t pcie_read_addr = 0x0000000001000000;
    uint32_t read_data = pcie_controller_target.read32(pcie_read_addr, &ok);
    SCML2_ASSERT_THAT(true, "PCIe read path executed without E126 error");
}
```
**Problem**: Test passes even if returned data is garbage!

### New Test Approach (Functional Verification)

All E2E tests now implement comprehensive functional verification:
- ✅ Configure TLB with known address mappings
- ✅ Prepare expected data at source
- ✅ Execute transaction through DUT
- ✅ Verify data integrity at destination
- ✅ Validate address translation correctness
- ✅ Test with multiple data patterns

**Example of New Test**:
```cpp
void testE2E_Inbound_PcieRead_TlbApp0_NocN() {
    // STEP 1: Configure TLB
    configure_tlb_entry_via_smn(0x18210000, 0, 0x80000000, 0x123);
    enable_system();
    
    // STEP 2: Write expected data to NOC
    uint32_t expected_data = 0xDEADBEEF;
    ok = noc_n_target.write32(0x80001000, expected_data);
    
    // STEP 3: Read via PCIe (should translate and route to NOC)
    uint32_t actual_data = pcie_controller_target.read32(0x0000000001001000, &ok);
    
    // STEP 4: VERIFY data integrity
    verify_data_match(expected_data, actual_data, "Inbound PCIe Read");
    
    // STEP 5: Test multiple patterns
    uint32_t patterns[] = {0x00000000, 0xFFFFFFFF, 0xAAAAAAAA, 0x55555555};
    for (size_t i = 0; i < 4; i++) {
        ok = noc_n_target.write32(0x80001000, patterns[i]);
        uint32_t read_back = pcie_controller_target.read32(0x0000000001001000, &ok);
        verify_data_match(patterns[i], read_back, "Pattern test");
    }
}
```

## Mandatory Verification Requirements

### Requirement 1: TLB Configuration

**Every test MUST configure relevant TLB entries before testing**

```cpp
// Configure TLB App In0[0]: PCIe addr 0x0000000001000000 -> NOC addr 0x80000000
uint32_t tlb_config_base = 0x18210000;  // TLB App In0 base
uint64_t noc_target_addr = 0x80000000;  // NOC physical address
configure_tlb_entry_via_smn(tlb_config_base, 0, noc_target_addr, 0x123);
```

### Requirement 2: System Enable

**Every test MUST enable the system before transactions**

```cpp
enable_system();  // Sets system_ready=1, enables=0x10001
```

### Requirement 3: Data Preparation

**Every test MUST prepare expected data before testing**

For **read tests**:
```cpp
// Write expected data to source before reading
uint32_t expected = 0xDEADBEEF;
ok = noc_n_target.write32(source_addr, expected);
```

For **write tests**:
```cpp
// Prepare unique test data
uint32_t test_data = 0xCAFEBABE;
// (will verify at destination after write)
```

### Requirement 4: Data Verification

**Every test MUST verify data integrity**

```cpp
uint32_t expected = 0x12345678;
uint32_t actual = /* read from destination */;

// Use helper function for detailed error reporting
verify_data_match(expected, actual, "Test Context");
```

**What verify_data_match() does**:
- Compares expected vs. actual data
- Reports detailed error with hex values if mismatch
- Provides context string for debugging
- Uses SCML2_ASSERT_THAT for proper test reporting

### Requirement 5: Address Translation Validation

**Every test MUST validate address translation occurred**

```cpp
// Example: PCIe write -> NOC read to verify translation
uint64_t pcie_write_addr = 0x0000000002000000;  // PCIe address
uint32_t write_data = 0xDEADBEEF;
ok = pcie_controller_target.write32(pcie_write_addr, write_data);

// Verify at TRANSLATED NOC address (TLB configured: PCIe 0x02000000 -> NOC 0x82000000)
uint64_t noc_translated_addr = 0x82000000;
uint32_t actual = noc_n_target.read32(noc_translated_addr, &ok);
verify_data_match(write_data, actual, "Translation Verification");
```

### Requirement 6: Multiple Pattern Testing

**Every test SHOULD test multiple data patterns (at least 3)**

```cpp
uint32_t patterns[] = {
    0x00000000,  // All zeros
    0xFFFFFFFF,  // All ones
    0xAAAAAAAA,  // Alternating bits
    0x55555555,  // Alternating bits (inverted)
    0x12345678,  // Sequential nibbles
    0xDEADBEEF,  // Known pattern
    0xCAFEBABE,  // Known pattern
    0xFEEDFACE   // Known pattern
};

for (size_t i = 0; i < sizeof(patterns)/sizeof(patterns[0]); i++) {
    // Write pattern
    ok = source.write32(addr, patterns[i]);
    
    // Read through DUT
    uint32_t read_back = destination.read32(addr, &ok);
    
    // Verify
    char context[128];
    snprintf(context, sizeof(context), "Pattern[%zu]=0x%08X", i, patterns[i]);
    verify_data_match(patterns[i], read_back, context);
}
```

## Test Structure Template

All E2E tests MUST follow this structure:

```cpp
void testE2E_<Category>_<TestName>() {
    bool ok = false;
    
    // STEP 1: Configure TLB(s) for this test
    configure_tlb_entry_via_smn(tlb_base, entry, phys_addr, attr);
    
    // STEP 2: Enable system
    enable_system();
    
    // STEP 3: Prepare expected data
    uint32_t expected_data = 0x...;
    ok = source.write32(source_addr, expected_data);
    SCML2_ASSERT_THAT(ok, "Data preparation failed");
    
    // STEP 4: Execute transaction through DUT
    uint32_t actual_data = destination.read32(dest_addr, &ok);
    SCML2_ASSERT_THAT(ok, "Transaction failed");
    
    // STEP 5: VERIFY data integrity
    verify_data_match(expected_data, actual_data, "Test Name");
    
    // STEP 6: Verify address translation (if applicable)
    // Read from translated address to confirm correct routing
    
    // STEP 7: Test multiple patterns
    test_data_patterns(write_addr, read_addr, "Test Name");
}
```

## Helper Functions Available

### enable_system()
```cpp
void enable_system() {
    bool ok = false;
    ok = smn_n_target.write32(0x18000000 + 0x0FFFC, 0x1);       // system_ready
    ok = smn_n_target.write32(0x18000000 + 0x0FFF8, 0x10001);   // enables
}
```

### verify_data_match()
```cpp
void verify_data_match(uint32_t expected, uint32_t actual, const char* context) {
    if (expected != actual) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s: DATA MISMATCH - expected 0x%08X, got 0x%08X", 
                 context, expected, actual);
        SCML2_ASSERT_THAT(false, msg);
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s: Data verified correctly: 0x%08X", context, expected);
        SCML2_ASSERT_THAT(true, msg);
    }
}
```

### write_and_verify_pattern()
```cpp
bool write_and_verify_pattern(uint64_t write_addr, uint64_t read_addr, 
                               uint32_t pattern, const char* path_name);
```

### test_data_patterns()
```cpp
void test_data_patterns(uint64_t write_addr, uint64_t read_addr, 
                        const char* test_name);
```

## Test Categories and Requirements

### Inbound Path Tests
- **Purpose**: Verify PCIe → NOC/SMN data flow
- **Must verify**: 
  - PCIe read returns correct NOC data
  - PCIe write reaches correct NOC address
  - TLB translation works correctly
  - Multiple BAR regions handled properly

### Outbound Path Tests
- **Purpose**: Verify NOC/SMN → PCIe data flow
- **Must verify**:
  - NOC read returns correct PCIe data
  - NOC write reaches correct PCIe address
  - Large address triggers correct TLB
  - DBI access works correctly

### Complete Flow Tests
- **Purpose**: Verify end-to-end memory operations
- **Must verify**:
  - Complete read transaction with data
  - Complete write transaction with data
  - Burst operations maintain data integrity
  - Config writes reach destination

### Concurrent Operation Tests
- **Purpose**: Verify simultaneous transactions
- **Must verify**:
  - Both paths maintain data integrity
  - No cross-talk between paths
  - Multiple TLB entries work independently

### Stress/Performance Tests
- **Purpose**: Verify system under heavy load
- **Must verify**:
  - Data integrity maintained during bursts
  - No dropped transactions
  - All TLB entries functional
  - Dynamic reconfiguration works

## Pass/Fail Criteria

### Test PASSES if:
1. ✅ All transactions complete successfully (ok == true)
2. ✅ All data verification checks pass (expected == actual)
3. ✅ Address translation verified correct
4. ✅ All pattern tests pass
5. ✅ No assertions fail

### Test FAILS if:
1. ❌ Transaction returns error
2. ❌ Data mismatch detected (expected != actual)
3. ❌ Data at wrong address (translation error)
4. ❌ Any pattern test fails
5. ❌ Any assertion fails

## Anti-Patterns (DO NOT DO THIS)

### ❌ BAD: Connectivity-Only Test
```cpp
void testBad() {
    uint32_t data = pcie_controller_target.read32(addr, &ok);
    SCML2_ASSERT_THAT(true, "Transaction completed");  // NO DATA CHECK!
}
```

### ❌ BAD: No TLB Configuration
```cpp
void testBad() {
    // Assumes default TLB config - WRONG!
    uint32_t data = pcie_controller_target.read32(addr, &ok);
    verify_data_match(expected, data, "test");
}
```

### ❌ BAD: No Data Preparation
```cpp
void testBad() {
    // Reading without writing expected data first
    uint32_t data = pcie_controller_target.read32(addr, &ok);
    // What should data be?? Test is meaningless!
}
```

### ❌ BAD: No Address Translation Check
```cpp
void testBad() {
    ok = pcie_controller_target.write32(pcie_addr, data);
    // Assumes write worked, but doesn't verify at translated NOC address
}
```

## Migration Guide

### Converting Old Tests to New Format

**Step 1**: Add TLB configuration
```cpp
// OLD: None
// NEW:
configure_tlb_entry_via_smn(0x18210000, 0, 0x80000000, 0x123);
enable_system();
```

**Step 2**: Add data preparation
```cpp
// OLD: None
// NEW:
uint32_t expected = 0xDEADBEEF;
ok = noc_n_target.write32(0x80001000, expected);
```

**Step 3**: Add data verification
```cpp
// OLD:
SCML2_ASSERT_THAT(true, "Transaction completed");

// NEW:
uint32_t actual = pcie_controller_target.read32(addr, &ok);
verify_data_match(expected, actual, "Test description");
```

**Step 4**: Add pattern testing
```cpp
// NEW:
test_data_patterns(write_addr, read_addr, "Test Name");
```

## Compliance Checklist

Use this checklist when writing/reviewing E2E tests:

- [ ] TLB entries configured before testing
- [ ] System enabled before transactions
- [ ] Expected data prepared at source
- [ ] Transaction execution with error checking
- [ ] Data verification at destination
- [ ] Address translation validated
- [ ] Multiple patterns tested (minimum 3)
- [ ] Clear test structure with STEP comments
- [ ] Meaningful assertion messages
- [ ] Helper functions used appropriately

## Conclusion

These verification requirements ensure that E2E tests provide **functional verification**, not just **connectivity verification**. 

Every E2E test must prove:
1. Data can flow through the path (connectivity)
2. Data arrives intact (integrity)
3. Data reaches the correct address (translation)
4. System works with various data patterns (robustness)

**Result**: Higher quality verification that catches real bugs!
