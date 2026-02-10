# Manual Test Harness for Keraunos PCIe Tile

## Overview

This manual test harness bypasses the SCML2 FastBuild auto-instrumentation framework to avoid double-binding issues with internal sub-module sockets. It provides comprehensive testing of the Keraunos PCIe Tile without relying on auto-generated test infrastructure.

## Why Manual Testing?

The SCML2 FastBuild coverage framework automatically attempts to bind monitors to ALL TLM sockets in the design hierarchy for coverage collection. This includes private sub-module sockets that are already bound internally, causing `E126: sc_export instance already bound` errors.

By using a manual test harness:
- ✅ Full control over socket bindings
- ✅ No auto-instrumentation conflicts
- ✅ Direct testing of functionality
- ✅ Clear, maintainable test cases

## Test Coverage

### 1. Reset Sequence
- Cold reset assertion and deassertion
- Warm reset handling
- PCIe controller reset

### 2. NOC-N Interface
- Read transactions
- Write transactions
- Response verification

### 3. SMN-N Interface
- Configuration read access
- Configuration write access
- Response validation

### 4. PCIe Inbound Data Path
- **PCIe → TLB → NOC**: Transactions with addresses routing to NOC
- **PCIe → TLB → SMN**: Transactions with addresses routing to SMN
- Address-based routing verification

### 5. PCIe Outbound Data Path
- **NOC → TLB → PCIe**: Outbound transactions from NOC
- **SMN → TLB → PCIe**: Outbound transactions from SMN

### 6. Address Routing
- Route 0x0 (TLB App 0)
- Route 0x1 (TLB App 1)
- Route 0x4 (TLB Sys)
- Route 0x8 (Bypass App)
- Route 0x9 (Bypass Sys)

### 7. Isolation Mode
- Transaction blocking during isolation
- Isolation enable/disable

### 8. Status Register
- Status register read access (route 0xE)
- Special address handling

## Building and Running

### Quick Start

```bash
# Navigate to manual test directory
cd /localdev/pdroy/keraunos_pcie_workspace/Keraunos_PCIe_tile/Tests/Manual

# Build and run tests
make run
```

### Build Only

```bash
make
# Or
make all
```

This creates the executable: `manual_test_keraunos_pcie`

### Run Tests

```bash
./manual_test_keraunos_pcie
```

Or use the make target:

```bash
make run
```

### Run with VCD Tracing

To generate waveform output for debugging:

```bash
make run-trace
```

This creates `manual_test.vcd` which can be viewed with:
- GTKWave
- ModelSim
- Other VCD-compatible waveform viewers

### Clean Build Artifacts

```bash
make clean
```

## Understanding Test Output

### Test Execution Flow

```
=== Keraunos PCIe Tile Manual Test Harness ===
Instantiating DUT...
Binding sockets...
Connecting control signals...
Registering target socket callbacks...
Initializing signals...
Initialization complete!

=== Starting Test Suite ===

--- Test 1: Reset Sequence ---
[PASS] Reset Sequence: Cold and warm resets completed

--- Test 2: NOC-N Read Access ---
[TEST→DUT] NOC-N read: addr=0x1000, cmd=READ, len=64
[PASS] NOC-N Read: Read transaction sent to NOC-N interface

... (more tests)

========================================
         TEST SUMMARY
========================================
Total Tests:  12
Passed:       12
Failed:       0
Success Rate: 100%
========================================
```

### Transaction Logging

Each transaction is logged with:
- **Direction**: `[TEST→DUT]` (test sending) or `[TEST←DUT]` (test receiving)
- **Interface**: NOC-N, SMN-N, or PCIe Controller
- **Address**: Transaction address in hex
- **Command**: READ or WRITE
- **Length**: Data length in bytes

Example:
```
[TEST→DUT] PCIe inbound (route to NOC): addr=0x10000, cmd=WRITE, len=8
[TEST←DUT] NOC-N transaction received: addr=0x10000, cmd=WRITE, len=8
```

### Test Results

Each test reports:
- **[PASS]**: Test executed successfully
- **[FAIL]**: Test encountered an error

Summary statistics at the end show:
- Total number of tests
- Pass/fail counts
- Success rate percentage
- Transaction counters for each interface

## Architecture

### Test Bench Structure

```
ManualTestBench (SC_MODULE)
├── DUT: KeraunosPcieTile
│   ├── noc_n_target      ← bound from test_noc_n_init
│   ├── noc_n_initiator   → bound to test_noc_n_tgt
│   ├── smn_n_target      ← bound from test_smn_n_init
│   ├── smn_n_initiator   → bound to test_smn_n_tgt
│   ├── pcie_controller_target      ← bound from test_pcie_ctrl_init
│   └── pcie_controller_initiator   → bound to test_pcie_ctrl_tgt
│
├── Test Initiators (send TO DUT):
│   ├── test_noc_n_init
│   ├── test_smn_n_init
│   └── test_pcie_ctrl_init
│
└── Test Targets (receive FROM DUT):
    ├── test_noc_n_tgt
    ├── test_smn_n_tgt
    └── test_pcie_ctrl_tgt
```

### Socket Bindings

**Inbound Path (Test → DUT):**
```
test_noc_n_init      → DUT.noc_n_target
test_smn_n_init      → DUT.smn_n_target
test_pcie_ctrl_init  → DUT.pcie_controller_target
```

**Outbound Path (DUT → Test):**
```
DUT.noc_n_initiator           → test_noc_n_tgt
DUT.smn_n_initiator           → test_smn_n_tgt
DUT.pcie_controller_initiator → test_pcie_ctrl_tgt
```

## Adding New Tests

To add a new test case:

1. **Declare the test method** in the `ManualTestBench` class:
```cpp
void test_13_my_new_test();
```

2. **Call it from `test_process()`**:
```cpp
void test_process() {
    // ... existing tests ...
    test_13_my_new_test();
    // ...
}
```

3. **Implement the test**:
```cpp
void test_13_my_new_test() {
    std::cout << "\n--- Test 13: My New Test ---" << std::endl;
    
    // Test logic here
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    send_write(test_noc_n_init, 0x1234, data, "My test transaction");
    
    wait(100, sc_core::SC_NS);
    
    bool passed = /* check condition */;
    log_test("My New Test", passed, "Description of what was tested");
}
```

## Debugging Tips

### Enable VCD Tracing

Run with tracing to see signal waveforms:
```bash
make run-trace
gtkwave manual_test.vcd
```

### Add Print Statements

In test methods, add detailed logging:
```cpp
std::cout << "[DEBUG] Current address: 0x" << std::hex << addr << std::dec << std::endl;
```

### Check Transaction Counters

At the end of each test, check:
```cpp
std::cout << "Transactions received: " << received_noc_transactions.size() << std::endl;
```

### Inspect Transaction Details

The test harness stores copies of all received transactions:
```cpp
for (auto* trans : received_noc_transactions) {
    std::cout << "NOC transaction: addr=0x" << std::hex << trans->get_address() << std::endl;
}
```

## Known Limitations

1. **Stub Routing Logic**: The current DUT implementation has stub routing methods that return `TLM_OK_RESPONSE` without full routing. Tests verify that transactions are accepted, but full end-to-end routing requires implementing the routing logic in the switches.

2. **No Coverage Metrics**: Unlike SCML2 FastBuild, this manual harness doesn't provide automatic coverage metrics. Coverage must be assessed manually by examining test output and transaction counts.

3. **Manual Test Maintenance**: Tests are not auto-generated. Any changes to the DUT interface require manual updates to the test harness.

## Troubleshooting

### Build Errors

**Issue**: `undefined reference to keraunos::pcie::KeraunosPcieTile::KeraunosPcieTile`

**Solution**: Ensure the library is built:
```bash
cd ../../SystemC/libso-gcc-9.5-64/FastBuild/F
# Check if libKeranous_pcie_tile.so exists
ls -la libKeranous_pcie_tile.so
```

If missing, rebuild via Platform Creator:
```bash
cd /localdev/pdroy/keraunos_pcie_workspace/Keraunos_PCIe_tile
/tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux/pc/bin/pctsh \
    Tool/PCT/Keranous_pcie_tile_build.tcl
```

### Runtime Errors

**Issue**: `error while loading shared libraries: libKeranous_pcie_tile.so`

**Solution**: Check library path or run via make:
```bash
make run  # Uses correct RPATH
```

**Issue**: Tests hang or don't complete

**Solution**: 
- Check for deadlocks in routing logic
- Verify all `wait()` statements have timeouts
- Enable debug output to see where it stops

### No Transactions Received

**Issue**: Transaction counters show 0

**Possible causes**:
1. **Routing not implemented**: Stub methods don't forward transactions
2. **Socket not bound**: Check binding code in constructor
3. **Transaction blocked**: Check isolation or enable signals

**Solution**: Add debug output in the DUT's routing methods to trace transaction flow.

## Next Steps

1. **Implement Full Routing Logic**: Update the stub methods in switches to implement proper routing based on address ranges.

2. **Add MSI Tests**: Extend tests to verify MSI interrupt generation and handling.

3. **Performance Testing**: Add tests for throughput and latency measurement.

4. **Error Injection**: Add tests that intentionally cause errors to verify error handling.

5. **Automated Regression**: Integrate with CI/CD pipeline for automated testing.

## Related Documentation

- `/localdev/pdroy/keraunos_pcie_workspace/FINAL_DIAGNOSIS.md` - Why manual testing is needed
- `/localdev/pdroy/keraunos_pcie_workspace/SCML2_FASTBUILD_BINDING_ISSUE.md` - SCML2 binding details
- `/localdev/pdroy/keraunos_pcie_workspace/CONNECTION_IMPLEMENTATION_SUMMARY.md` - Module connections

## Support

For issues or questions:
1. Check test output for error messages
2. Enable VCD tracing for debugging
3. Review the source code in `manual_test_harness.cpp`
4. Consult the DUT implementation in `../../SystemC/src/`

---

**Status**: ✅ Ready to use  
**Last Updated**: 2026-02-03  
**Author**: AI Assistant (based on Keraunos PCIe Tile design)
