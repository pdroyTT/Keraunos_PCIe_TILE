# Keraunos PCIE Tile Testbench

This directory contains the testbench implementation for the Keraunos PCIE Tile SystemC/TLM2.0 models.

## Directory Structure

```
tb/
├── include/          # Testbench header files
│   ├── tb_common.h                    # Common test utilities
│   ├── tb_tlb_reference_model.h       # TLB reference models
│   ├── tb_stimulus_generator.h        # Stimulus generators
│   ├── tb_monitor.h                   # Transaction monitors
│   ├── tb_testcases.h                 # Test case declarations
│   └── tb_testbench.h                 # Top-level testbench
├── src/              # Testbench source files
│   ├── tb_testbench.cpp               # Testbench implementation
│   ├── tb_testcases_inbound.cpp       # Inbound TLB test cases
│   ├── tb_testcases_outbound.cpp      # Outbound TLB test cases
│   ├── tb_testcases_msi.cpp           # MSI Relay test cases
│   ├── tb_testcases_switch.cpp        # Switch test cases
│   ├── tb_testcases_config.cpp        # Config register test cases
│   ├── tb_testcases_clock_reset.cpp   # Clock/reset test cases
│   └── tb_main.cpp                    # Main test runner
└── README.md         # This file
```

## Test Infrastructure

### Test Framework Components

1. **TestCase Base Class** (`tb_common.h`)
   - Base class for all test cases
   - Provides pass/fail tracking
   - Supports setup/teardown methods

2. **Reference Models** (`tb_tlb_reference_model.h`)
   - Golden models implementing specification algorithms
   - Used for comparison with DUT output
   - Includes models for all TLB types

3. **Stimulus Generators** (`tb_stimulus_generator.h`)
   - `TlbStimulusGenerator`: Generates TLM transactions for TLB testing
   - `ApbStimulusGenerator`: Generates APB transactions for register access

4. **Monitors** (`tb_monitor.h`)
   - `TransactionMonitor`: Captures and records transactions
   - Provides transaction history and analysis

5. **Scoreboard** (`tb_common.h`)
   - Compares DUT output with reference model
   - Tracks mismatches and errors

## Test Cases

### Inbound TLB Tests
- `TC_INBOUND_SYS_001`: Basic Address Translation
- `TC_INBOUND_SYS_002`: Invalid Entry Detection
- `TC_INBOUND_SYS_003`: Index Calculation
- `TC_INBOUND_SYS_004`: Page Boundary Crossing
- `TC_INBOUND_SYS_005`: AxUSER Field Mapping
- `TC_INBOUND_SYS_006`: Concurrent Transactions

### Outbound TLB Tests
- `TC_OUTBOUND_SYS_001`: Basic Outbound Translation
- `TC_OUTBOUND_APP0_001`: App Outbound Translation

### MSI Relay Tests
- `TC_MSI_RELAY_001`: Basic MSI Reception
- `TC_MSI_RELAY_004`: MSI-X Support

### Switch Tests
- `TC_SWITCH_NOC_PCIE_001`: Address Routing
- `TC_SWITCH_NOC_PCIE_002`: Status Register Special Routing

### Config Register Tests
- `TC_CONFIG_REG_001`: Register Read/Write
- `TC_CONFIG_REG_002`: Register Reset Values

### Clock/Reset Tests
- `TC_CLOCK_RESET_001`: Cold Reset
- `TC_CLOCK_RESET_003`: Clock Generation

## Building and Running

### Prerequisites
- SystemC library
- SCML2 library
- TLM2.0 library
- C++ compiler with C++11 support

### Build Instructions

```bash
# From workspace root
mkdir -p build/tb
cd build/tb

# Compile testbench
g++ -std=c++11 -I../../include -I../../tb/include \
    -I/path/to/systemc/include -I/path/to/scml2/include \
    ../../tb/src/*.cpp ../../src/*.cpp \
    -L/path/to/systemc/lib -L/path/to/scml2/lib \
    -lsystemc -lscml2 -o tb_keraunos_pcie

# Run tests
./tb_keraunos_pcie
```

### Running Individual Test Cases

The testbench can be modified to run specific test cases by commenting out unwanted tests in `tb_main.cpp`.

## Test Coverage

The testbench implements test cases from the test plan document (`doc/Keraunos_PCIE_Tile_Testplan.md`). Current implementation includes:

- Basic functionality tests
- Error handling tests
- Edge case tests
- Integration tests (partial)

## Extending the Testbench

### Adding a New Test Case

1. Declare the test case class in `tb/include/tb_testcases.h`
2. Implement the test case in appropriate `tb/src/tb_testcases_*.cpp` file
3. Register the test case in `tb/src/tb_main.cpp`

Example:
```cpp
class TC_New_Test : public TestCase {
public:
    TC_New_Test(Testbench* tb) : TestCase("TC_NEW_TEST"), testbench_(tb) {}
    void run() override {
        // Test implementation
        pass();
    }
private:
    Testbench* testbench_;
};
```

### Adding a New Reference Model

1. Create a new class inheriting from `TlbReferenceModel` in `tb/include/tb_tlb_reference_model.h`
2. Implement the lookup methods according to specification
3. Instantiate in testbench and use in test cases

## Notes

- Some test cases are implemented as placeholders and need to be completed with actual DUT interaction
- Socket connections in the testbench are simplified and may need adjustment based on actual DUT interfaces
- Reference models implement the specification algorithms but may need refinement based on actual behavior

