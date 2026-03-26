# SII and CII Implementation Details

## Overview

This document provides detailed implementation information for the System Information Interface (SII) block and Configuration Intercept Interface (CII) in the Keraunos PCIe Tile.

## Architecture

### SII Block Purpose

The SII block serves three main functions:

1. **Configuration Provider**: Provides configuration information to the PCIe Controller IP (bus numbers, device type, etc.)
2. **Configuration Tracker**: Monitors configuration updates via CII interface
3. **Interrupt Generator**: Generates interrupts to SMC PLIC when configuration changes are detected

### CII Interface Purpose

The Configuration Intercept Interface (CII) is a monitoring interface from the PCIe Controller that reports when configuration registers are written. This allows the SII block to track which configuration registers have been modified by the host processor.

## Implementation Details

### 1. Configuration Update Tracking

#### CII Tracking Process

The SII block tracks configuration updates using the following logic:

```cpp
// Combinational process (runs continuously)
void cii_tracking_process() {
    cii_modified_ = 0;  // Initialize
    
    // Check if CII reports a config write
    if (cii_hv && cii_hdr_type == 0x04 && cii_hdr_addr[11:7] == 0) {
        // Extract register index from address[6:2]
        reg_index = cii_hdr_addr[6:2];
        cii_modified_[reg_index] = 1;  // Mark as modified
    }
}
```

**Key Points:**
- Only tracks first 128B of config space (address[11:7] == 0)
- Type 0x04 (00100b) indicates configuration write transaction
- Each bit in `cii_modified_` corresponds to one 32-bit config register
- This is combinational logic - updates immediately when CII reports a write

#### Configuration Modified Register Update

The `cfg_modified_` register is updated sequentially (on PCIe core clock):

```cpp
// Sequential process (PCIE core clock domain)
void cfg_modified_update_process() {
    if (reset_n == 0) {
        cfg_modified_sync_ = 0;
    } else {
        // RW1C semantics: clear bits where software wrote 1, set bits from CII
        cfg_modified_sync_ = (cfg_modified_sync_ & ~cii_clear_) | cii_modified_;
        
        // Generate interrupt if any bit is set
        config_int = cfg_modified_sync_.or_reduce();
    }
}
```

**RW1C Semantics:**
- **Read**: Returns current modified bits
- **Write 1**: Clears the corresponding bit
- **Write 0**: No effect
- **CII Update**: Sets the corresponding bit when config register is written

### 2. Updating Controller IP Configuration

#### Configuration Flow

```
SMC Firmware (via APB, AXI clock)
    ↓
SII Register Write (AXI clock domain)
    ↓
Clock Domain Crossing (AXI → PCIe core clock)
    ↓
SII Output Signals (PCIE core clock domain)
    ↓
PCIe Controller IP (PCIE core clock)
```

#### Key Configuration Registers

1. **Core Control Register** (`CORE_CONTROL_OFFSET = 0x0000`)
   - `[2:0]` Device Type: 0=EP, 4=RP
   - Drives `device_type` output signal

2. **Bus/Device Number Register** (`BUS_DEV_NUM_OFFSET = 0x0008`)
   - `[7:0]` Device Number
   - `[15:8]` Bus Number
   - Drives `app_bus_num` and `app_dev_num` output signals

3. **Configuration Modified Register** (`CFG_MODIFIED_OFFSET = 0x0004`)
   - RW1C register tracking which config registers were modified
   - Read by firmware to determine what changed

#### Clock Domain Crossing

The SII block implements clock domain crossing between:
- **AXI Clock Domain**: APB accesses from SMC firmware
- **PCIE Core Clock Domain**: Interface to PCIe Controller IP

**CDC Implementation:**
```cpp
// APB → PCIe Core Clock
void cdc_apb_to_pcie() {
    // Synchronize register values
    core_control_pcie_ = core_control_axi_;
    bus_dev_num_pcie_ = bus_dev_num_axi_;
    
    // Drive outputs to PCIe Controller
    device_type = (core_control_axi_ & DEVICE_TYPE_MASK) == RP;
    app_bus_num = (bus_dev_num_axi_ >> 8) & 0xFF;
    app_dev_num = bus_dev_num_axi_ & 0xFF;
}
```

**Note:** In a real implementation, proper CDC synchronizers (e.g., 2-stage synchronizers) would be used to prevent metastability.

### 3. Interrupt Generation and Routing to SMC PLIC

#### Interrupt Generation

The interrupt is generated when any configuration register modification is detected:

```cpp
// In cfg_modified_update_process (PCIE core clock domain)
config_int.write(cfg_modified_sync_.or_reduce());
```

**Interrupt Behavior:**
- Asserted when `cfg_modified_sync_` has any bit set (any register modified)
- Deasserted when all bits are cleared (via RW1C writes)
- Active high signal

#### Interrupt Routing Path

```
SII Block (PCIE core clock)
    ↓ config_int
Top-Level Tile
    ↓ config_update (connected in keraunos_pcie_tile.cpp)
External Interface
    ↓
SMC PLIC (Platform-Level Interrupt Controller)
    ↓
SMC Firmware Interrupt Handler
```

**Connection in Top-Level Tile:**
```cpp
// In keraunos_pcie_tile.cpp::connect_components()
sii_block_->config_int(config_update);  // Routes to top-level output
```

The top-level `config_update` signal is one of the interrupt outputs listed in Table 5 of the specification, which is routed to the SMC PLIC.

#### Firmware Handling

When firmware receives the interrupt:

1. **Read `cfg_modified` register** via APB to determine which registers changed
2. **Process the changes** (e.g., update internal state, reconfigure other components)
3. **Clear the modified bits** by writing 1 to corresponding bits in `cfg_modified` register (RW1C)
4. **Interrupt deasserts** when all bits are cleared

**Example Firmware Flow:**
```c
// Interrupt handler
void sii_config_int_handler() {
    uint32_t modified = read_sii_reg(CFG_MODIFIED_OFFSET);
    
    // Check which registers were modified
    if (modified & (1 << CFG_SUBBUS_NUM_REG)) {
        // Handle sub-bus number change
        handle_subbus_change();
    }
    
    // Clear all modified bits (RW1C)
    write_sii_reg(CFG_MODIFIED_OFFSET, modified);
    
    // Interrupt will deassert when all bits are cleared
}
```

## Register Map

### SII Block Address Map

| Offset | Size | Name | Access | Description |
|--------|------|------|--------|-------------|
| 0x0000 | 4B | Core Control | R/W | Device type, control bits |
| 0x0004 | 4B | Config Modified | R/W1C | Configuration modified tracking |
| 0x0008 | 4B | Bus/Dev Number | R/W | Bus and device number assignment |
| ... | ... | ... | ... | Other SII registers |

**Base Address:** 0x18100000 + 0x04000 (via SMN-IO APB demux)

### CII Interface Signals

| Signal | Width | Direction | Description |
|--------|-------|-----------|-------------|
| `cii_hv` | 1-bit | Input | CII Header Valid |
| `cii_hdr_type` | 5-bit | Input | Header Type [4:0] (0x04 = config write) |
| `cii_hdr_addr` | 12-bit | Input | Header Address [11:0] |

## Timing Considerations

### Clock Domains

1. **AXI Clock Domain** (~400MHz)
   - APB accesses from SMC firmware
   - Register writes/reads

2. **PCIE Core Clock Domain** (~1GHz)
   - CII interface monitoring
   - Configuration output signals to PCIe Controller
   - Interrupt generation

### CDC Requirements

- All register writes from APB must be synchronized to PCIe core clock
- `cfg_modified` register reads from APB must be synchronized from PCIe core clock
- Proper synchronizers required to prevent metastability

## Testing Considerations

### Test Scenarios

1. **Configuration Write Tracking**
   - Write to config register via host processor
   - Verify CII reports the write
   - Verify `cfg_modified` bit is set
   - Verify interrupt is asserted

2. **RW1C Functionality**
   - Set multiple `cfg_modified` bits
   - Write 1 to clear specific bits
   - Verify only written bits are cleared
   - Verify interrupt deasserts when all bits cleared

3. **Clock Domain Crossing**
   - Write register via APB (AXI clock)
   - Verify output signals update correctly (PCIE clock)
   - Verify no metastability issues

4. **Interrupt Routing**
   - Generate config update interrupt
   - Verify interrupt reaches top-level output
   - Verify interrupt can be cleared via register write

## References

- Keraunos PCIE Tile Specification v0.7.023, Section 2.2.1.2
- SystemC/TLM2.0 Design Document, Section 4.6

