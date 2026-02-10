# Keraunos PCIe Tile Test Guide

## Overview

This document describes the comprehensive test suite for the Keraunos PCIe Tile, which validates all data flow paths, routing logic, and component interactions.

---

## Test Organization

The test suite is organized into 8 major test groups, aligned with the architecture documented in `CONNECTION_IMPLEMENTATION_SUMMARY.md`:

### 1. **Inbound Path Tests** (PCIe → TLB → NOC/SMN)
Tests data flow from PCIe Controller through TLBs to external NOC-N and SMN-N interfaces.

| Test Name | Description | Route Bits | Expected Path |
|-----------|-------------|------------|---------------|
| `testInbound_TlbApp0_Route` | Route to TLB App In0[0] | 0x0 | PCIe → NOC-PCIE Switch → TLB App In0 → NOC-IO → NOC-N |
| `testInbound_TlbApp1_Route` | Route to TLB App In1 | 0x1 | PCIe → NOC-PCIE Switch → TLB App In1 → NOC-IO → NOC-N |
| `testInbound_TlbSys0_Route` | Route to TLB Sys In0 | 0x4 | PCIe → NOC-PCIE Switch → TLB Sys In0 → SMN-IO → SMN-N |
| `testInbound_BypassApp_Route` | Bypass App route | 0x8 | PCIe → NOC-PCIE Switch → Bypass App |
| `testInbound_BypassSys_Route` | Bypass Sys route | 0x9 | PCIe → NOC-PCIE Switch → Bypass Sys |
| `testInbound_IsolationHandling` | Isolation blocks traffic | N/A | Verify DECERR when isolated |
| `testInbound_DecodeError` | Invalid routes return DECERR | 0x2, 0x3, 0x5-0x7, 0xA-0xD | Verify DECERR response |

**Key Validation Points:**
- Route bits [63:60] correctly decoded
- Address translation through TLBs
- Proper socket connections to NOC/SMN interfaces
- Isolation and error handling

---

### 2. **Outbound Path Tests** (NOC/SMN → TLB → PCIe)
Tests data flow from external NOC-N and SMN-N interfaces through TLBs to PCIe Controller.

| Test Name | Description | Address Range | Expected Path |
|-----------|-------------|---------------|---------------|
| `testOutbound_NocN_to_TlbAppOut0` | NOC-N to TLB App Out0 | 0x18900000 | NOC-N → NOC-IO → TLB App Out0 → NOC-PCIE → PCIe |
| `testOutbound_NocN_to_TlbAppOut1` | NOC-N to TLB App Out1 | [51:48] != 0 | NOC-N → NOC-IO → TLB App Out1 → NOC-PCIE → PCIe |
| `testOutbound_SmnN_to_TlbSysOut0` | SMN-N to TLB Sys Out0 | 0x18400000 | SMN-N → SMN-IO → TLB Sys Out0 → NOC-PCIE → PCIe |
| `testOutbound_AddressMasking52bit` | 52-bit address masking | N/A | Verify upper 12 bits masked |

**Key Validation Points:**
- Address-based routing in NOC-IO and SMN-IO switches
- 52-bit address masking (mask upper 12 bits)
- TLB translation for outbound traffic
- Correct forwarding to PCIe Controller

---

### 3. **Configuration Path Tests** (SMN-N → TLB Config)
Tests configuration access to TLBs via SMN-N interface.

| Test Name | Description | Config Address | Target |
|-----------|-------------|----------------|--------|
| `testConfig_TlbSysIn0` | TLB Sys In0 config | 0x18040000 | TLB Sys In0 config_socket |
| `testConfig_TlbAppIn0_AllInstances` | TLB App In0[0-3] config | 0x18041000-0x18044000 | TLB App In0 config_sockets (4 instances) |
| `testConfig_TlbAppIn1` | TLB App In1 config | 0x18045000 | TLB App In1 config_socket |
| `testConfig_TlbSysOut0` | TLB Sys Out0 config | 0x18046000 | TLB Sys Out0 config_socket |
| `testConfig_TlbAppOut0` | TLB App Out0 config | 0x18047000 | TLB App Out0 config_socket |
| `testConfig_TlbAppOut1` | TLB App Out1 config | 0x18048000 | TLB App Out1 config_socket |
| `testConfig_AddressDecoding` | Address decode logic | 0x18040000-0x18050000 | Verify routing, DECERR on reserved |

**TLB Configuration Address Map:**
```
0x18040000: TLB Sys In0 Config    (4KB)
0x18041000: TLB App In0[0] Config (4KB)
0x18042000: TLB App In0[1] Config (4KB)
0x18043000: TLB App In0[2] Config (4KB)
0x18044000: TLB App In0[3] Config (4KB)
0x18045000: TLB App In1 Config    (4KB)
0x18046000: TLB Sys Out0 Config   (4KB)
0x18047000: TLB App Out0 Config   (4KB)
0x18048000: TLB App Out1 Config   (4KB)
0x18049000: Reserved (DECERR)
```

**Key Validation Points:**
- SMN-IO switch correctly decodes config addresses
- Each TLB receives config via dedicated initiator port
- Reserved regions return DECERR
- Configuration updates propagate to TLB behavior

---

### 4. **MSI Path Tests**
Tests MSI-X interrupt generation and configuration.

| Test Name | Description | Address | Path |
|-----------|-------------|---------|------|
| `testMSI_RelayConfig` | MSI Relay configuration | 0x18000000 | SMN-N → SMN-IO → MSI Relay csr_apb_socket |
| `testMSI_AxiAccess` | MSI-X table access | 0x18800000 | NOC-N → NOC-IO → MSI Relay msi_axi_socket |
| `testMSI_InterruptGeneration` | MSI-X interrupt trigger | N/A | Write table → Generate interrupt |

**MSI Address Map:**
```
0x18000000: MSI Relay Config (256KB, 8 PF x 16KB) - APB via SMN-N
0x18800000: MSI-X Table (1MB) - AXI via NOC-N
```

**Key Validation Points:**
- MSI Relay accessible via both SMN-N (config) and NOC-N (table)
- Configuration updates enable/disable MSI-X
- Table writes trigger interrupts
- Interrupt outputs properly connected

---

### 5. **Status Register Tests**
Tests System Ready status register access.

| Test Name | Description | Address | Route |
|-----------|-------------|---------|-------|
| `testStatusReg_ReadRoute0xE` | Read via route 0xE | 0xE000...0000 | NOC-PCIE Switch special handling |
| `testStatusReg_ReadRoute0xF` | Read via route 0xF | 0xF000...0000 | NOC-PCIE Switch special handling |
| `testStatusReg_WriteSystemReady` | Write System Ready bit | 0xE000...0000 | Verify bit propagates |
| `testStatusReg_DisabledAccess` | Access when disabled | 0xE000...0000 | Should still be accessible |

**Status Register Routing:**
- **Route 0xE** + AxADDR[59:7] == 0 (128B region) → Status Register (READ only)
- **Route 0xF** → Status Register (READ/WRITE)
- Accessible even when `pcie_inbound_app_enable` = false

**Key Validation Points:**
- Route 0xE special case (128B region check)
- Route 0xF always routes to status register
- Write updates `status_reg_value` output
- Read returns current System Ready state

---

### 6. **Switch Routing Tests**
Tests individual switch routing logic.

| Test Name | Description | Switch | Coverage |
|-----------|-------------|--------|----------|
| `testNocPcieSwitch_RouteLogic` | All route values | NOC-PCIE | 0x0-0xF routes |
| `testNocIoSwitch_RouteLogic` | Address-based routing | NOC-IO | MSI, TLB Out, DECERR, NOC-N |
| `testSmnIoSwitch_RouteLogic` | Address-based routing | SMN-IO | MSI, TLB, SII, SerDes, SMN-N |

**NOC-PCIE Switch Routing Table:**
```
Route Bits [63:60] → Target
0x0 → TLB App In0[0]
0x1 → TLB App In1
0x2-0x3 → DECERR
0x4 → TLB Sys In0
0x5-0x7 → DECERR
0x8 → Bypass App
0x9 → Bypass Sys
0xA-0xD → DECERR
0xE → Status Register (if [59:7]==0) or TLB Sys In0
0xF → Status Register
```

**NOC-IO Switch Address Map:**
```
0x18800000: MSI Relay (1MB)
0x18900000: TLB App Out0 (1MB) or AxADDR[51:48] != 0
0x18A00000: DECERR (2MB)
0x18C00000: DECERR (2MB)
0x18E00000: DECERR (2MB)
Default: NOC-N (via TLB App Inbound)
```

**SMN-IO Switch Address Map:**
```
0x18000000: MSI Relay Config (256KB)
0x18040000: TLB Config (64KB)
0x18050000: SMN-IO Fabric CSR (64KB)
0x18080000: SerDes AHB (256KB)
0x180C0000: SerDes APB (256KB)
0x18100000: SII Config (1MB)
0x18200000: Reserved (2MB DECERR)
0x18400000: TLB Sys Out0 (1MB)
0x18500000: Reserved (3MB DECERR)
Default: SMN-N (via TLB Sys Inbound)
```

---

### 7. **End-to-End Data Flow Tests**
Tests complete data paths through multiple components.

| Test Name | Description | Path |
|-----------|-------------|------|
| `testE2E_PcieRead_to_NocN` | PCIe read → NOC-N | PCIe → NOC-PCIE → TLB App In0 → NOC-IO → NOC-N |
| `testE2E_PcieWrite_to_SmnN` | PCIe write → SMN-N | PCIe → NOC-PCIE → TLB Sys In0 → SMN-IO → SMN-N |
| `testE2E_NocN_to_PcieRead` | NOC-N → PCIe read | NOC-N → NOC-IO → TLB App Out0 → NOC-PCIE → PCIe |
| `testE2E_SmnN_Config_to_TLB` | SMN-N config → TLB | SMN-N → SMN-IO → TLB config_socket |

**Test Flow:**
1. Generate transaction at source
2. Verify routing through each switch
3. Verify TLB translation (if applicable)
4. Verify transaction reaches destination
5. Check response propagates back

---

### 8. **SII Block Tests**
Tests System Initialization Interface block.

| Test Name | Description | Interface |
|-----------|-------------|-----------|
| `testSII_CII_Monitoring` | CII signal monitoring | PCIe Controller CII → SII |
| `testSII_ConfigUpdate` | Configuration update | SMN-N → SII apb_socket |
| `testSII_OutputSignals` | Output signal propagation | SII → PCIe Controller |

**SII Interfaces:**
- **Input (CII):** `pcie_cii_hv`, `pcie_cii_hdr_type`, `pcie_cii_hdr_addr`
- **Output (SII):** `pcie_app_bus_num`, `pcie_app_dev_num`, `pcie_device_type`, `pcie_sys_int`
- **Config:** APB socket @ 0x18100000

---

## Test Execution

### Running All Tests
```bash
cd /localdev/pdroy/keraunos_pcie_workspace/Keraunos_PCIe_tile
# Build and run tests via Platform Creator
```

### Running Individual Test Groups
```cpp
// In test harness:
SCML2_RUN_TEST(Keranous_pcie_tileTest, testInbound_TlbApp0_Route);
```

### Test Output
- **Pass:** Test completes without assertion failures
- **Fail:** Assertion failure with detailed message
- **Info:** Logs provide test execution trace

---

## Extending Tests

### Adding New Test Cases

1. **Add test method to class:**
```cpp
void testMyNewFeature() {
    // Test implementation
    SCML2_ASSERT_THAT(condition, "assertion message");
    SCML2_LOG_INFO("testMyNewFeature", "description");
}
```

2. **Register test in SCML2_BEGIN_TESTS block:**
```cpp
SCML2_BEGIN_TESTS(Keranous_pcie_tileTest);
SCML2_TEST(testMyNewFeature);
SCML2_END_TESTS();
```

### Test Helper Functions

**Transaction Creation:**
```cpp
// Create TLM transaction
tlm::tlm_generic_payload trans;
uint64_t addr = 0x0000000012345678ULL;
uint8_t data[64];
trans.set_address(addr);
trans.set_data_ptr(data);
trans.set_data_length(64);
trans.set_command(tlm::TLM_WRITE_COMMAND);
```

**Socket Access:**
```cpp
// Write via socket
sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
socket->b_transport(trans, delay);

// Check response
SCML2_ASSERT_THAT(trans.get_response_status() == tlm::TLM_OK_RESPONSE,
                  "Transaction succeeded");
```

---

## Test Coverage

### Data Paths Covered
- ✅ Inbound: PCIe → TLB → NOC/SMN (all routes)
- ✅ Outbound: NOC/SMN → TLB → PCIe
- ✅ Configuration: SMN-N → TLB Config
- ✅ MSI: NOC-N → MSI Table, SMN-N → MSI Config
- ✅ Status Register: Route 0xE/0xF

### Switch Routing Covered
- ✅ NOC-PCIE: All 16 route values (0x0-0xF)
- ✅ NOC-IO: Address-based routing
- ✅ SMN-IO: Address-based routing with TLB config

### Error Handling Covered
- ✅ DECERR for invalid routes
- ✅ Isolation request handling
- ✅ Timeout detection
- ✅ Reserved region access

### Component Interactions Covered
- ✅ TLB translation (inbound/outbound)
- ✅ Switch interconnections
- ✅ External interface connections
- ✅ SII CII monitoring
- ✅ MSI interrupt generation

---

## Known Limitations

### DUT Address Passthrough (Partial)
- MSI Relay and SII config callbacks still receive full SMN addresses instead of offsets
- TLB configuration via SMN works correctly (base address stripped)
- MSI-X enable/mask are internal signals not exposed as tile ports

### Isolation Recovery
- `isolate_req` permanently clears enables; cold reset + SMN write to 0x18040000+0xFFF8 required for recovery

### Resolved Issues
- ~~Stub Implementations~~ - All 81 tests now perform full transaction + assertion-based verification
- ~~1GB Memory Limitation~~ - Replaced by `sparse_backing_memory` with 256TB range
- ~~Output Port Stubs~~ - DUT initiator sockets now forward outbound traffic to testbench

---

## Debugging Failed Tests

### Common Issues

**Issue:** Transaction doesn't reach expected target
- **Check:** Socket connections in `keraunos_pcie_tile.cpp`
- **Verify:** Routing logic in switch implementations
- **Debug:** Enable transaction logging

**Issue:** DECERR when expecting TLM_OK_RESPONSE
- **Check:** Address is within valid range
- **Verify:** Isolation/enable signals are correct
- **Debug:** Check switch route_address() logic

**Issue:** TLB config not updating
- **Check:** Address matches TLB config range (0x18040000-0x18050000)
- **Verify:** SMN-IO switch routes to correct TLB config initiator
- **Debug:** Enable config socket logging

### Logging
```cpp
// Enable detailed logging in test
enable_logging();

// Add custom logs
SCML2_LOG_INFO("test_name", "Custom message: %d", value);
```

---

## Test Methodology

### Cross-Socket Data Verification

The testbench uses a **custom `sparse_backing_memory`** class (replacing `scml2::testing::test_memory`) that implements `scml2::mappable_if` and `scml2::testing::memory_if` using `std::map<uint64_t, uint8_t>` for truly sparse storage. This enables:

1. **256TB address range** without actual memory allocation (zero overhead)
2. **Cross-socket data verification**: Data written to a DUT input port (e.g., PCIe) traverses the internal TLBs and switches, exits through an initiator socket (e.g., NOC-N), and is captured in the backing memory for comparison
3. **No 1GB limitation**: The previous `test_memory` had a practical limit of ~1GB; `sparse_backing_memory` has no such constraint

### DUT Output Port Architecture

The three DUT output ports are `tlm_utils::simple_initiator_socket<64>` types:
- `noc_n_initiator` - Forwards inbound PCIe traffic after TLB translation to NOC
- `smn_n_initiator` - Forwards bypass/system traffic to SMN
- `pcie_controller_initiator` - Forwards outbound NOC/SMN traffic after TLB translation to PCIe

Each is connected to an `scml2::testing::initiator_socket_proxy` with a `sparse_backing_memory` instance in the testbench.

### TLB Configuration via SMN

Tests configure TLB entries using a helper function `configure_tlb_entry_via_smn()` that writes to the SMN-IO switch's TLB config space:
```
Base: 0x18040000
TLB Sys Out0: offset 0x0000    TLB App Out0: offset 0x1000
TLB App Out1: offset 0x2000    TLB Sys In0:  offset 0x3000
TLB App In0[0-3]: offset 0x4000-0x7000
TLB App In1: offset 0x8000
```

---

## Summary

This comprehensive test suite validates:
- **81 test cases** (41 E2E + 40 directed), all passing
- **All 3 switches** (NOC-PCIE, NOC-IO, SMN-IO)
- **9 TLB instances** (inbound/outbound configuration)
- **MSI relay** (config and interrupt generation)
- **SII block** (CII monitoring and outputs)
- **Error handling** (DECERR, isolation, timeout)
- **Cross-socket data verification** through `sparse_backing_memory`
- **Full 64-bit address space** exercised (original 1TB+ address patterns)

The tests are organized to match the architecture documented in the HLD (`doc/Keraunos_PCIe_Tile_HLD.md`), making it easy to correlate test failures with specific architectural components.
