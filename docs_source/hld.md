# Keraunos PCIe Tile - High-Level Design

**Version:** 2.0  
**Date:** February 10, 2026  
**Status:** Updated - DUT initiator socket architecture  

---

## 1. Introduction

### 1.1 Purpose

This document describes the high-level architecture of the Keraunos PCIe Tile, a SystemC/TLM2.0 model that provides PCIe connectivity between Network-on-Chip (NOC), System Management Network (SMN), and PCIe controller interfaces.

### 1.2 Scope

The Keraunos PCIe Tile integrates:
- **3 fabric switches** for routing traffic between NOC, SMN, and PCIe domains
- **8 Translation Lookaside Buffers (TLBs)** for address translation (5 inbound, 3 outbound)
- **MSI-X relay unit** for interrupt handling
- **System Information Interface (SII)** for PCIe configuration tracking
- **Configuration register block** for tile-level control
- **Clock and reset control** infrastructure

### 1.3 Key Terms

| Term | Definition |
|------|------------|
| **CII** | Configuration Intercept Interface - PCIe config space write detection |
| **DECERR** | Decode Error - TLM address error response |
| **MSI-X** | Message Signaled Interrupts - Extended |
| **NOC** | Network-on-Chip |
| **PBA** | Pending Bit Array (for MSI-X) |
| **SII** | System Information Interface |
| **SMN** | System Management Network |
| **TLB** | Translation Lookaside Buffer |
| **TLM** | Transaction Level Modeling |

---

## 2. Architecture Overview

### 2.1 Top-Level Architecture

The Keraunos PCIe Tile acts as a bridge between three major interconnect domains:

```{mermaid}
graph TB
    subgraph "External Interfaces"
        NOC[NOC Network<br/>Target/Initiator]
        SMN[SMN Network<br/>Target/Initiator]
        PCIE[PCIe Controller<br/>Target/Initiator]
    end
    
    subgraph "Keraunos PCIe Tile"
        subgraph "Data Path"
            NOCP[NOC-PCIE<br/>Switch]
            NOCI[NOC-IO<br/>Switch]
            SMNI[SMN-IO<br/>Switch]
            TLBI[Inbound TLBs<br/>5 instances]
            TLBO[Outbound TLBs<br/>3 instances]
            MSI[MSI Relay<br/>Unit]
        end
        
        subgraph "Control Path"
            CFG[Config<br/>Registers]
            SII[SII Block<br/>CII Tracking]
            CLK[Clock/Reset<br/>Control]
        end
    end
    
    NOC -->|Data Traffic| NOCI
    NOCI --> TLBO --> NOCP --> PCIE
    PCIE --> NOCP --> TLBI --> NOCI --> NOC
    
    SMN -->|Config Access| SMNI
    SMNI --> CFG
    SMNI --> SII
    SMNI --> TLBI
    SMNI --> TLBO
    SMNI --> MSI
    
    PCIE -->|CII Signals| SII
    NOC -->|MSI Input| MSI
    
    CLK -.->|Reset/Clock| NOCP
    CLK -.->|Reset/Clock| NOCI
    CLK -.->|Reset/Clock| SMNI
    
    style NOCP fill:#e1f5ff
    style NOCI fill:#e1f5ff
    style SMNI fill:#e1f5ff
    style TLBI fill:#fff4e1
    style TLBO fill:#fff4e1
    style MSI fill:#ffe1f5
    style SII fill:#ffe1f5
    style CFG fill:#e1ffe1
    style CLK fill:#e1ffe1
```

### 2.2 Design Principles

1. **C++ Implementation:** All internal components are C++ classes (not SystemC modules) for performance and simplicity.
2. **Function Callbacks:** Internal communication uses C++ function callbacks instead of TLM sockets.
3. **RAII Memory Management:** Smart pointers (`std::unique_ptr`) for automatic resource cleanup.
4. **Address-Based Routing:** Upper address bits [63:60] determine routing paths.
5. **Signal-Based Control:** Control plane uses `sc_signal` for isolation, reset, and configuration tracking.

### 2.3 System Connection Diagrams

This section shows how the Keraunos PCIe Tile connects to external subsystems in a complete SoC integration. Each subsystem connection is shown in a separate diagram for clarity.

#### 2.3.1 SMN Subsystem Connection

```{mermaid}
graph LR
    subgraph SMN["SMN Subsystem"]
        direction TB
        SMN_FAB[SMN Network Fabric<br/>Configuration Master<br/>64-bit TLM]
        SMN_INT[SMN Interrupt Controller]
    end
    
    subgraph TILE["Keraunos PCIe Tile"]
        direction TB
        
        subgraph TLM["TLM Sockets"]
            SMN_T[smn_n_target<br/>64-bit Target]
            SMN_I[smn_n_initiator<br/>64-bit Initiator]
        end
        
        subgraph OUT["Output Signals"]
            CFG_UP[config_update]
            BUS[pcie_app_bus_num<br/>8-bit]
            DEV[pcie_app_dev_num<br/>8-bit]
            TYPE[pcie_device_type<br/>1-bit: 0=EP, 1=RP]
        end
    end
    
    SMN_FAB <-->|TLM blocking transport<br/>Configuration writes<br/>Read status/TLBs/MSI| SMN_T
    SMN_I <-->|TLM blocking transport<br/>Bypass path responses| SMN_FAB
    CFG_UP -->|Config space update<br/>interrupt from CII| SMN_INT
    BUS -.->|Read from SII| SMN_FAB
    DEV -.->|Read from SII| SMN_FAB
    TYPE -.->|Read from SII| SMN_FAB
    
    style SMN_T fill:#ffe1f5,stroke:#e91e63,stroke-width:3px
    style SMN_I fill:#ffe1f5,stroke:#e91e63,stroke-width:3px
    style CFG_UP fill:#ffe1e1,stroke:#f44336,stroke-width:3px
```

**SMN Connection Details:**

| Signal/Socket | Type | Width | Direction | Purpose |
|---------------|------|-------|-----------|---------|
| `smn_n_target` | TLM Socket | 64-bit | Input | Receives configuration transactions from SMN for TLBs, MSI, SII, Config registers |
| `smn_n_initiator` | TLM Initiator Socket | 64-bit | Output | Sends bypass path responses via `tlm_utils::simple_initiator_socket<64>` to `sparse_backing_memory` |
| `config_update` | Signal | 1-bit | Output | Interrupt to SMN when PCIe config space is written (CII tracking) |
| `pcie_app_bus_num` | Signal | 8-bit | Output | PCIe bus number from SII CORE_CONTROL register |
| `pcie_app_dev_num` | Signal | 8-bit | Output | PCIe device number from SII CORE_CONTROL register |
| `pcie_device_type` | Signal | 1-bit | Output | Device type: 0=Endpoint, 1=Root Port |

**SMN Configuration Targets:**

SMN can configure the following via `smn_n_target`:
- **Config Register Block** (offset 0x0xxx): `system_ready`, enables, isolation control
- **SII Block** (offset 0x1xxx): CORE_CONTROL, CFG_MODIFIED (RW1C), BUS_DEV_NUM
- **TLB Configuration** (offset 0x3xxx-0x7xxx): Valid bits, page masks, base addresses
- **MSI Relay** (offset 0x8xxx): MSI-X table, PBA, masks

**Configuration Sequence:**
1. SMN writes `system_ready=1` to Config Register Block
2. SMN configures TLB entries (inbound/outbound)
3. SMN enables data paths: `pcie_inbound_app_enable=1`, `pcie_outbound_app_enable=1`
4. SMN monitors `config_update` interrupt for PCIe config space changes

#### 2.3.2 NOC Subsystem Connection

```{mermaid}
graph LR
    subgraph NOC["NOC Subsystem"]
        direction TB
        NOC_FAB[NOC Network Fabric<br/>Data Transfer<br/>64-bit TLM]
        MSI_SRC[MSI Interrupt Source<br/>Vector Generator]
        TO_MON[Timeout Monitor]
    end
    
    subgraph TILE["Keraunos PCIe Tile"]
        direction TB
        
        subgraph TLM["TLM Sockets"]
            NOC_T[noc_n_target<br/>64-bit Target]
            NOC_I[noc_n_initiator<br/>64-bit Initiator]
        end
        
        subgraph OUT["Output Signals"]
            TO_OUT[noc_timeout<br/>3-bit]
        end
    end
    
    NOC_FAB <-->|TLM blocking transport<br/>Outbound data path<br/>NOC to PCIe via TLB| NOC_T
    MSI_SRC -->|Write MSI vector<br/>addr 0x18800000<br/>Generate MSI-X| NOC_T
    NOC_I <-->|TLM blocking transport<br/>Inbound data path<br/>PCIe to NOC via TLB| NOC_FAB
    TO_OUT -.->|Timeout status<br/>monitoring| TO_MON
    
    style NOC_T fill:#e1f5ff,stroke:#3498db,stroke-width:3px
    style NOC_I fill:#e1f5ff,stroke:#3498db,stroke-width:3px
    style TO_OUT fill:#fff4e1,stroke:#ff9800,stroke-width:2px
```

**NOC Connection Details:**

| Signal/Socket | Type | Width | Direction | Purpose |
|---------------|------|-------|-----------|---------|
| `noc_n_target` | TLM Socket | 64-bit | Input | Receives NOC transactions for outbound PCIe path (NOC→TLB→PCIe) |
| `noc_n_initiator` | TLM Initiator Socket | 64-bit | Output | Sends translated inbound PCIe transactions via `tlm_utils::simple_initiator_socket<64>` to `sparse_backing_memory` (PCIe→TLB→NOC) |
| `noc_timeout` | Signal | 3-bit | Output | NOC transaction timeout status for monitoring |

**Data Flow Paths:**

1. **Outbound Path (NOC → PCIe):**
   - NOC writes to `noc_n_target` socket
   - NOC-IO switch routes to outbound TLB
   - TLB translates NOC address to PCIe address
   - NOC-PCIE switch checks `pcie_outbound_app_enable`
   - If enabled, forwards through `pcie_controller_initiator` (real `tlm_utils::simple_initiator_socket<64>`) to `sparse_backing_memory` for cross-socket data verification

2. **Inbound Path (PCIe → NOC):**
   - PCIe controller writes to `pcie_controller_target` socket
   - NOC-PCIE switch routes based on addr[63:60]
   - Inbound TLB translates PCIe address to NOC address
   - NOC-IO switch forwards through `noc_n_initiator` (real `tlm_utils::simple_initiator_socket<64>`) to `sparse_backing_memory` for cross-socket data verification

3. **MSI Generation:**
   - NOC MSI source writes MSI vector to `noc_n_target` at special address (0x18800000)
   - MSI Relay Unit processes vector, checks masks
   - Generates MSI-X transaction to PCIe controller
   - **Note:** MSI generation functional via initiator sockets to `sparse_backing_memory`

#### 2.3.3 Designware PCIe Controller Connection

```{mermaid}
graph LR
    subgraph DW["Designware PCIe Controller"]
        direction TB
        
        subgraph DATA["Data Path"]
            AXI_M[AXI Master Interface<br/>Inbound TLPs]
            AXI_S[AXI Slave Interface<br/>Outbound TLPs]
        end
        
        subgraph CII["CII Interface"]
            CII_HV[cii_hdr_valid]
            CII_TYPE[cii_hdr_type<br/>5-bit]
            CII_ADDR[cii_hdr_addr<br/>12-bit]
        end
        
        subgraph INT["Interrupts"]
            FLR_I[FLR Request]
            HOT_I[Hot Reset]
            RAS_I[RAS Error]
            DMA_I[DMA Done]
            MISC_I[Misc Int]
        end
        
        subgraph CTL["Control"]
            CLK[pcie_core_clk]
            RST[controller_reset_n]
        end
    end
    
    subgraph TILE["Keraunos PCIe Tile"]
        direction TB
        
        subgraph TLM["TLM Sockets"]
            P_TGT[pcie_controller_target<br/>64-bit Target]
            P_INIT[pcie_controller_initiator<br/>64-bit Initiator]
        end
        
        subgraph CII_IN["CII Inputs"]
            C_HV[pcie_cii_hv]
            C_TYPE[pcie_cii_hdr_type<br/>5-bit]
            C_ADDR[pcie_cii_hdr_addr<br/>12-bit]
        end
        
        subgraph INT_IN["Interrupt Inputs"]
            F_IN[pcie_flr_request]
            H_IN[pcie_hot_reset]
            R_IN[pcie_ras_error]
            D_IN[pcie_dma_completion]
            M_IN[pcie_misc_int]
        end
        
        subgraph INT_OUT["Interrupt Outputs"]
            F_OUT[function_level_reset]
            H_OUT[hot_reset_requested]
            R_OUT[ras_error]
            D_OUT[dma_completion]
            M_OUT[controller_misc_int]
        end
        
        subgraph CTL_IN["Control Inputs"]
            P_CLK[pcie_core_clk]
            P_RST[pcie_controller_reset_n]
        end
    end
    
    subgraph SYS["System"]
        INT_CTRL[System Interrupt<br/>Controller]
    end
    
    AXI_M <-->|TLM b_transport<br/>Inbound PCIe TLPs| P_TGT
    P_INIT <-->|TLM b_transport<br/>Outbound PCIe TLPs| AXI_S
    
    CII_HV -->|Config write detected| C_HV
    CII_TYPE -->|TLP type 0x04| C_TYPE
    CII_ADDR -->|Byte offset 0-127| C_ADDR
    
    FLR_I -->|Event signal| F_IN
    HOT_I -->|Event signal| H_IN
    RAS_I -->|Event signal| R_IN
    DMA_I -->|Event signal| D_IN
    MISC_I -->|Event signal| M_IN
    
    F_OUT -->|Forwarded| INT_CTRL
    H_OUT -->|Forwarded| INT_CTRL
    R_OUT -->|Forwarded| INT_CTRL
    D_OUT -->|Forwarded| INT_CTRL
    M_OUT -->|Forwarded| INT_CTRL
    
    CLK -->|PCIe domain| P_CLK
    RST -->|Active low| P_RST
    
    style P_TGT fill:#fff4e1,stroke:#ff9800,stroke-width:3px
    style P_INIT fill:#fff4e1,stroke:#ff9800,stroke-width:3px
    style C_HV fill:#d1f5d1,stroke:#4caf50,stroke-width:2px
    style C_TYPE fill:#d1f5d1,stroke:#4caf50,stroke-width:2px
    style C_ADDR fill:#d1f5d1,stroke:#4caf50,stroke-width:2px
    style F_OUT fill:#ffe1e1,stroke:#f44336,stroke-width:2px
    style H_OUT fill:#ffe1e1,stroke:#f44336,stroke-width:2px
    style R_OUT fill:#ffe1e1,stroke:#f44336,stroke-width:2px
    style D_OUT fill:#ffe1e1,stroke:#f44336,stroke-width:2px
    style M_OUT fill:#ffe1e1,stroke:#f44336,stroke-width:2px
```

**PCIe Controller Connection Details:**

| Signal/Socket | Type | Width | Direction | Purpose |
|---------------|------|-------|-----------|---------|
| `pcie_controller_target` | TLM Socket | 64-bit | Input | Receives inbound TLPs from PCIe controller AXI master |
| `pcie_controller_initiator` | TLM Initiator Socket | 64-bit | Output | Sends outbound TLPs via `tlm_utils::simple_initiator_socket<64>` to `sparse_backing_memory` for cross-socket data verification |
| `pcie_cii_hv` | Signal | 1-bit | Input | CII header valid: config space write detected |
| `pcie_cii_hdr_type` | Signal | 5-bit | Input | TLP type (0x04 = configuration write) |
| `pcie_cii_hdr_addr` | Signal | 12-bit | Input | Config space byte address (track first 128B) |
| `pcie_flr_request` | Signal | 1-bit | Input | Function-level reset requested |
| `pcie_hot_reset` | Signal | 1-bit | Input | Hot reset detected on link |
| `pcie_ras_error` | Signal | 1-bit | Input | RAS error event |
| `pcie_dma_completion` | Signal | 1-bit | Input | DMA transaction completed |
| `pcie_misc_int` | Signal | 1-bit | Input | Miscellaneous controller interrupt |
| `function_level_reset` | Signal | 1-bit | Output | FLR forwarded to system (requires delta cycle) |
| `hot_reset_requested` | Signal | 1-bit | Output | Hot reset forwarded to system |
| `ras_error` | Signal | 1-bit | Output | RAS error forwarded to system |
| `dma_completion` | Signal | 1-bit | Output | DMA done forwarded to system |
| `controller_misc_int` | Signal | 1-bit | Output | Misc interrupt forwarded to system |
| `pcie_core_clk` | Clock | 1-bit | Input | PCIe core clock (Gen3: 250MHz) |
| `pcie_controller_reset_n` | Signal | 1-bit | Input | Controller reset (active low), resets SII CII state |

**CII Tracking Flow:**
1. PCIe controller detects config space write
2. Asserts `cii_hdr_valid` with type and address
3. Tile's SII block tracks writes to first 128 bytes
4. Sets bit in `cfg_modified` register
5. Asserts `config_update` interrupt to SMN
6. SMN reads `CFG_MODIFIED`, clears with RW1C writes

**Interrupt Forwarding:**
- **Critical:** All interrupt forwarding requires `sc_core::wait(SC_ZERO_TIME)` after input changes
- Input signals must propagate through SystemC delta cycle before outputs are valid
- Outputs go to system interrupt controller for software handling

#### 2.3.4 System Control Connection

```{mermaid}
graph LR
    subgraph SYS["System Control"]
        direction TB
        RST_CTRL[Reset Controller]
        ISO_CTRL[Isolation Controller<br/>Power Management]
        CLK_GEN[Clock Generator]
        INT_CTRL[Interrupt Controller]
    end
    
    subgraph TILE["Keraunos PCIe Tile"]
        direction TB
        
        subgraph RST["Reset Inputs"]
            COLD[cold_reset_n<br/>Active Low]
            WARM[warm_reset_n<br/>Active Low]
        end
        
        subgraph ISO["Isolation"]
            ISO_REQ[isolate_req]
        end
        
        subgraph CLK["Clocks"]
            AXI[axi_clk]
        end
        
        subgraph INT["Interrupt Outputs"]
            SYS_I[pcie_sys_int]
        end
    end
    
    RST_CTRL -->|Cold reset<br/>Resets SII and<br/>reset control| COLD
    RST_CTRL -->|Warm reset<br/>Similar to cold| WARM
    ISO_CTRL -->|Isolation request<br/>Clears all enables<br/>PERMANENT| ISO_REQ
    CLK_GEN -->|AXI/NOC domain<br/>System clock| AXI
    SYS_I -.->|System interrupt<br/>Currently tied to 0| INT_CTRL
    
    style COLD fill:#ffe1e1,stroke:#f44336,stroke-width:2px
    style WARM fill:#ffe1e1,stroke:#f44336,stroke-width:2px
    style ISO_REQ fill:#ffcccc,stroke:#cc0000,stroke-width:3px
    style AXI fill:#e1ffe1,stroke:#4caf50,stroke-width:2px
```

**System Control Connection Details:**

| Signal | Type | Width | Direction | Purpose |
|--------|------|-------|-----------|---------|
| `cold_reset_n` | Signal | 1-bit | Input | Cold reset (active low): Resets SII block and reset control module |
| `warm_reset_n` | Signal | 1-bit | Input | Warm reset (active low): Similar behavior to cold reset |
| `isolate_req` | Signal | 1-bit | Input | Isolation request: Clears `system_ready` and all enables (PERMANENT) |
| `axi_clk` | Clock | 1-bit | Input | AXI/NOC domain clock (system-dependent frequency) |
| `pcie_sys_int` | Signal | 1-bit | Output | System interrupt output (currently tied to 0) |

**Reset Behavior:**

| Reset Type | Resets | Does NOT Reset | Recovery |
|------------|--------|----------------|----------|
| `cold_reset_n` | SII CII tracking, reset control module | Config registers, TLBs, enables | Software reconfiguration required |
| `warm_reset_n` | Similar to cold reset | Config registers, TLBs, enables | Software reconfiguration required |
| `pcie_controller_reset_n` | SII CII tracking only | Everything else | Automatic on reset deassertion |

**Critical Isolation Limitation:**

⚠️ **WARNING:** Asserting `isolate_req` permanently clears all enables. Recovery requires:
1. Cold reset cycle (`cold_reset_n` low then high)
2. SMN writes `system_ready=1`
3. SMN reconfigures TLB entries
4. SMN writes `pcie_inbound_app_enable=1`
5. SMN writes `pcie_outbound_app_enable=1`

Simply deasserting `isolate_req` does NOT restore functionality. This is a known architectural limitation.

**Connection Summary:**

#### TLM Socket Connections (64-bit, blocking transport)

| Tile Socket | Direction | External Subsystem | Purpose |
|-------------|-----------|-------------------|---------|
| `noc_n_target` | Input | NOC Network Fabric | Receives NOC transactions for outbound PCIe (NOC→PCIe) |
| `noc_n_initiator` | Output | NOC / `sparse_backing_memory` | Sends translated inbound PCIe via real initiator socket (PCIe→NOC) |
| `smn_n_target` | Input | SMN Network Fabric | Receives configuration transactions from SMN |
| `smn_n_initiator` | Output | SMN / `sparse_backing_memory` | Sends bypass/response transactions via real initiator socket |
| `pcie_controller_target` | Input | DW PCIe AXI Master | Receives inbound TLPs from PCIe controller |
| `pcie_controller_initiator` | Output | DW PCIe AXI Slave / `sparse_backing_memory` | Sends outbound TLPs via real initiator socket |

#### Clock Connections

| Tile Input | Source | Frequency | Purpose |
|------------|--------|-----------|---------|
| `pcie_core_clk` | DW PCIe Controller | PCIe Link Speed | PCIe domain clock (Gen3: 250MHz) |
| `axi_clk` | System Clock Generator | System Dependent | AXI/NOC domain clock |

**Note:** Both clocks must be running for tile operation. Clock domain crossing is handled internally.

#### Reset Connections

| Tile Input | Source | Type | Effect |
|------------|--------|------|--------|
| `cold_reset_n` | System Reset Controller | Active Low | Resets SII and reset control modules |
| `warm_reset_n` | System Reset Controller | Active Low | Similar to cold reset |
| `pcie_controller_reset_n` | DW PCIe Controller | Active Low | Resets SII CII tracking state |

**Integration Note:** Cold reset does NOT clear configuration registers. Software reconfiguration required after reset.

#### CII Interface Connections (Configuration Intercept)

| Tile Input | DW PCIe Output | Width | Purpose |
|------------|----------------|-------|---------|
| `pcie_cii_hv` | `cii_hdr_valid` | 1-bit | Indicates valid CII header (config write detected) |
| `pcie_cii_hdr_type` | `cii_hdr_type` | 5-bit | TLP type (0x04 = configuration write) |
| `pcie_cii_hdr_addr` | `cii_hdr_addr` | 12-bit | Config space byte address (track first 128B) |

**Purpose:** Allows tile to track PCIe configuration space writes and generate `config_update` interrupt to SMN.

#### Interrupt Forwarding (PCIe Controller → Tile → System)

**Inputs from DW PCIe Controller:**

| Tile Input | DW PCIe Source | Trigger Condition |
|------------|----------------|-------------------|
| `pcie_flr_request` | FLR logic | Function-level reset requested via config space |
| `pcie_hot_reset` | Link training | Hot reset detected on PCIe link |
| `pcie_ras_error` | RAS logic | Reliability/Availability/Serviceability error |
| `pcie_dma_completion` | DMA engine | DMA transaction completed |
| `pcie_misc_int` | Various | Controller-specific miscellaneous interrupts |

**Outputs to System Interrupt Controller:**

| Tile Output | Destination | Description |
|-------------|-------------|-------------|
| `function_level_reset` | System Interrupt Controller | Forwarded FLR (requires delta cycle wait) |
| `hot_reset_requested` | System Interrupt Controller | Forwarded hot reset (requires delta cycle wait) |
| `ras_error` | System Interrupt Controller | Forwarded RAS error (requires delta cycle wait) |
| `dma_completion` | System Interrupt Controller | Forwarded DMA done (requires delta cycle wait) |
| `controller_misc_int` | System Interrupt Controller | Forwarded misc interrupt (requires delta cycle wait) |

**Implementation Note:** All interrupt forwarding requires `sc_core::wait(SC_ZERO_TIME)` after input signal changes to allow SystemC delta cycle propagation before reading outputs.

#### CII Interrupt Output

| Tile Output | Destination | Trigger Condition |
|-------------|-------------|-------------------|
| `config_update` | SMN Interrupt Controller | PCIe config space write detected in first 128B (via CII tracking) |

**Software Handling:** SMN reads `CFG_MODIFIED` register, processes changes, writes 1s to clear (RW1C). Interrupt deasserts when all bits cleared.

#### Status Outputs

| Tile Output | Destination | Width | Description |
|-------------|-------------|-------|-------------|
| `pcie_app_bus_num` | System/SMN | 8-bit | PCIe bus number from SII CORE_CONTROL |
| `pcie_app_dev_num` | System/SMN | 8-bit | PCIe device number from SII CORE_CONTROL |
| `pcie_device_type` | System/SMN | 1-bit | 0=Endpoint, 1=Root Port (from SII) |
| `pcie_sys_int` | System Interrupt Controller | 1-bit | System interrupt (currently tied to 0) |
| `noc_timeout` | NOC Timeout Monitor | 3-bit | NOC transaction timeout status |

#### Isolation Control

| Tile Input | Source | Purpose |
|------------|--------|---------|
| `isolate_req` | Isolation Controller | Request isolation mode (clears all enables) |

**Critical Limitation:** Deasserting `isolate_req` does NOT restore enables. Recovery requires:
1. Cold reset cycle
2. SMN reconfiguration of `system_ready`, `pcie_inbound_app_enable`, `pcie_outbound_app_enable`

#### MSI Input (Special Case)

The NOC subsystem can write to the tile's `noc_n_target` socket at a specific MSI input address (e.g., 0x18800000) to generate MSI-X interrupts. The MSI relay unit processes these and forwards them to the PCIe controller via `pcie_controller_initiator`.

**Note:** MSI routing paths are functional. Outbound MSI transactions are forwarded through real initiator sockets to `sparse_backing_memory`, enabling cross-socket data verification in the testbench.

#### Integration Checklist

✅ **Required Connections:**
- [ ] Connect all 6 TLM sockets (3 target, 3 initiator) with matching bit widths
- [ ] Connect both clocks (`pcie_core_clk` from DW PCIe, `axi_clk` from system)
- [ ] Connect all 3 reset signals (cold, warm, pcie_controller)
- [ ] Connect 3 CII interface signals (hv, type, addr) from DW PCIe CII output
- [ ] Connect 5 interrupt inputs from DW PCIe controller
- [ ] Connect 6 interrupt outputs to system interrupt controller(s)
- [ ] Connect isolation control from power management
- [ ] Connect status outputs to system monitoring/SMN

⚠️ **SystemC Simulation Requirements:**
- Ensure `sc_core::wait(SC_ZERO_TIME)` in drivers after changing input signals
- All interrupt outputs require delta cycle propagation
- TLM blocking transport calls are synchronous but internal signals are not

🔧 **Configuration Sequence:**
1. Deassert cold reset
2. Wait for stable clocks
3. SMN writes `system_ready=1`
4. SMN configures TLB entries
5. SMN writes `pcie_inbound_app_enable=1` and `pcie_outbound_app_enable=1`
6. System enters normal operation

---

## 3. Theory of Operation

This section describes the operational modes and transaction flows of the Keraunos PCIe Tile.

### 3.1 Operational Modes

The PCIe Tile operates in several distinct modes:

#### Normal Operation Mode
- **System Ready**: `system_ready` register bit set to 1
- **Application Paths Enabled**: `pcie_inbound_app_enable` and `pcie_outbound_app_enable` set as needed
- **No Isolation**: `isolate_req` signal deasserted
- **Clocks Running**: Both `pcie_core_clk` and `axi_clk` active
- **No Reset**: `cold_reset_n` and `warm_reset_n` asserted (active high)

In this mode, all data paths are operational and transactions flow freely between NOC, PCIe, and SMN domains.

#### Isolation Mode
- **Triggered By**: Assertion of `isolate_req` signal
- **Effect**: Clears `system_ready`, `pcie_inbound_app_enable`, and `pcie_outbound_app_enable`
- **Data Path**: All PCIe controller transactions return `TLM_ADDRESS_ERROR_RESPONSE` (DECERR)
- **Recovery Limitation**: Deasserting `isolate_req` does NOT automatically restore enables (requires cold reset + software reconfiguration)
- **Use Case**: Power domain isolation, fault containment

#### Bypass Mode
- **Condition**: `system_ready=0` but `pcie_inbound_app_enable=1` or `pcie_outbound_app_enable=1`
- **Routing**: Uses route 0xE or 0xF to access status register directly
- **Purpose**: Allow status monitoring without full system initialization
- **Limited Functionality**: Only status register readable; normal data paths blocked

#### Reset Modes

**Cold Reset:**
- **Trigger**: `cold_reset_n` signal deasserted (active low)
- **Scope**: Resets SII block (`pcie_sii_reset_ctrl_`) and reset control module (`pcie_reset_ctrl_`)
- **Does NOT Reset**: Configuration registers, TLB entries, switch enables
- **Recovery**: Requires software reconfiguration after reset

**Warm Reset:**
- **Trigger**: `warm_reset_n` signal deasserted (active low)
- **Scope**: Similar to cold reset but may preserve certain configuration state
- **Note**: Current implementation behavior similar to cold reset

### 3.2 Initialization and Boot Sequence

```{mermaid}
sequenceDiagram
    participant SMN as SMN Host
    participant Tile as PCIe Tile
    participant Switch as NOC-PCIE Switch
    participant TLB as TLB Units
    participant SII as SII Block
    participant PCIE as PCIe Controller

    Note over Tile: Power-On / Cold Reset
    SMN->>Tile: Deassert cold_reset_n
    Tile->>SII: Reset SII state
    Tile->>Switch: system_ready=0, enables=0
    
    Note over Tile: Configuration Phase
    SMN->>Switch: Write system_ready=1
    Switch->>Switch: Enable routing logic
    
    SMN->>TLB: Configure inbound TLB entries
    Note over TLB: Set valid bits, page masks, base addresses
    
    SMN->>TLB: Configure outbound TLB entries
    Note over TLB: Set application path translations
    
    SMN->>Switch: Write pcie_inbound_app_enable=1
    SMN->>Switch: Write pcie_outbound_app_enable=1
    Switch->>Switch: Enable application data paths
    
    Note over Tile: Normal Operation
    PCIE->>Switch: Inbound transaction
    Switch->>TLB: Address translation
    TLB->>Switch: Translated address
    Switch->>PCIE: Transaction complete
```

**Initialization Steps:**

1. **Reset Phase**
   - Assert `cold_reset_n` to bring tile to known state
   - SII block clears all CII tracking state
   - Switch enables default to 0

2. **System Ready Configuration**
   - SMN writes `system_ready=1` to status register (offset 0x0)
   - Enables basic routing logic in switches
   - Status register becomes readable via routes 0xE and 0xF

3. **TLB Configuration**
   - Configure inbound TLBs (Sys In0, Sys In1, App In0/1/2)
   - Set valid bits, page sizes, base addresses, address masks
   - Configure outbound TLBs (Sys Out, App Out0/1)
   - Set AxUSER fields for NOC routing

4. **Data Path Enablement**
   - Write `pcie_inbound_app_enable=1` for PCIe→NOC application paths
   - Write `pcie_outbound_app_enable=1` for NOC→PCIe application paths
   - System enters normal operation mode

### 3.3 Inbound Data Flow (PCIe to NOC)

**Transaction Path: PCIe Controller → NOC-PCIE Switch → Inbound TLB → NOC-IO Switch → NOC**

```{mermaid}
sequenceDiagram
    participant PCIE as PCIe Controller
    participant NOCP as NOC-PCIE Switch
    participant TLB as Inbound TLB
    participant NOCI as NOC-IO Switch
    participant NOC as NOC Network

    PCIE->>NOCP: b_transport(addr[63:0], data)
    Note over NOCP: Check system_ready
    
    alt system_ready == 0
        NOCP->>PCIE: TLM_ADDRESS_ERROR_RESPONSE
    else Route bits [63:60]
        NOCP->>NOCP: Decode route (0x0,0x1,0x4,0x6,0x7)
        
        alt Application path (0x0, 0x1, 0x6, 0x7)
            Note over NOCP: Check pcie_inbound_app_enable
            alt Enable == 0
                NOCP->>PCIE: TLM_ADDRESS_ERROR_RESPONSE
            else Enable == 1
                NOCP->>TLB: translate_and_forward(addr)
                Note over TLB: Calculate index = (addr >> shift) & page_mask
                Note over TLB: Check valid bit
                
                alt valid == 0
                    TLB->>NOCP: TLM_ADDRESS_ERROR_RESPONSE
                    NOCP->>PCIE: DECERR
                else valid == 1
                    TLB->>TLB: new_addr = entry.addr + offset
                    TLB->>TLB: Set AxUSER field
                    TLB->>NOCI: forward(new_addr, AxUSER)
                    NOCI->>NOC: b_transport(translated_addr)
                    NOC->>NOCI: TLM_OK_RESPONSE
                    NOCI->>TLB: Success
                    TLB->>NOCP: Success
                    NOCP->>PCIE: TLM_OK_RESPONSE
                end
            end
        else System path (0x4)
            NOCP->>TLB: translate_and_forward(addr)
            Note over TLB: System TLB (smaller pages)
            TLB->>NOCI: forward(translated_addr)
            NOCI->>NOC: b_transport
            NOC->>PCIE: Response propagates back
        end
    end
```

**Key Steps:**

1. **PCIe Controller Transaction**
   - Transaction arrives at `pcie_controller_target` socket
   - 64-bit address with route field [63:60]

2. **NOC-PCIE Switch Routing**
   - Check `system_ready` flag (blocks if 0)
   - Decode route bits [63:60]
   - For application paths: check `pcie_inbound_app_enable`
   - For system paths: always allowed if `system_ready=1`

3. **TLB Translation**
   - Extract page index: `(addr >> shift) & page_mask`
   - Look up TLB entry
   - Check `valid` bit
   - Calculate translated address: `base_addr + offset`
   - Set AxUSER field for NOC routing

4. **NOC-IO Switch Forwarding**
   - Forward translated address through `noc_n_initiator` (real `tlm_utils::simple_initiator_socket<64>`) to `sparse_backing_memory`
   - Preserve AxUSER field
   - Cross-socket data verification: data written via inbound path can be read back via outbound path
   - Return response to PCIe controller

### 3.4 Outbound Data Flow (NOC to PCIe)

**Transaction Path: NOC → NOC-IO Switch → Outbound TLB → NOC-PCIE Switch → PCIe Controller**

```{mermaid}
sequenceDiagram
    participant NOC as NOC Network
    participant NOCI as NOC-IO Switch
    participant TLB as Outbound TLB
    participant NOCP as NOC-PCIE Switch
    participant PCIE as PCIe Controller

    NOC->>NOCI: b_transport(noc_addr, data)
    NOCI->>TLB: translate_address()
    
    Note over TLB: Calculate TLB index
    Note over TLB: Check valid bit
    
    alt valid == 0
        TLB->>NOCI: TLM_ADDRESS_ERROR_RESPONSE
        NOCI->>NOC: DECERR
    else valid == 1
        TLB->>TLB: pcie_addr = entry.addr + offset
        TLB->>TLB: Convert 64-bit to 52-bit
        
        alt Application path
            Note over NOCP: Check pcie_outbound_app_enable
            alt Enable == 0
                NOCP->>NOCI: TLM_ADDRESS_ERROR_RESPONSE
                NOCI->>NOC: DECERR
            else Enable == 1
                NOCP->>PCIE: b_transport(pcie_addr)
                PCIE->>NOCP: TLM_OK_RESPONSE
                NOCP->>NOCI: Success
                NOCI->>NOC: TLM_OK_RESPONSE
            end
        else System path
            NOCP->>PCIE: b_transport(pcie_addr)
            PCIE->>NOC: Response propagates back
        end
    end
```

**Key Steps:**

1. **NOC Transaction**
   - Arrives at `noc_n_target` socket
   - NOC-specific addressing

2. **NOC-IO Switch Processing**
   - Routes to appropriate outbound TLB

3. **Outbound TLB Translation**
   - Translate NOC address to PCIe address space
   - Check valid bit
   - Convert 64-bit internal address to 52-bit PCIe address

4. **NOC-PCIE Switch Gating**
   - For application paths: check `pcie_outbound_app_enable`
   - If enabled, forward through `pcie_controller_initiator` (real `tlm_utils::simple_initiator_socket<64>`) to `sparse_backing_memory`
   - Cross-socket data verification: data written via outbound path can be read back via inbound path

### 3.5 Configuration Access Flow

**Transaction Path: SMN → SMN-IO Switch → Configuration Targets**

```{mermaid}
sequenceDiagram
    participant SMN as SMN Host
    participant SMNI as SMN-IO Switch
    participant TGT as Config Target
    participant SII as SII Block
    participant MSI as MSI Relay
    participant TLB as TLB Config

    SMN->>SMNI: b_transport(smn_addr, data, cmd)
    
    Note over SMNI: Decode address bits [31:28]
    
    alt Config Register Block
        SMNI->>TGT: forward(offset 0x0-0xFFF)
        TGT->>TGT: Read/Write register
        Note over TGT: system_ready, enables, isolation
        TGT->>SMNI: Response
        SMNI->>SMN: TLM_OK_RESPONSE
        
    else SII Configuration
        SMNI->>SII: process_apb_access(addr, data)
        Note over SII: CORE_CONTROL, CFG_MODIFIED, BUS_DEV_NUM
        alt Write to CFG_MODIFIED (RW1C)
            SII->>SII: Clear modified bits
            SII->>SII: Update config_update interrupt
        end
        SII->>SMNI: Response
        
    else MSI Relay Configuration
        SMNI->>MSI: process_msi_config(addr, data)
        Note over MSI: Configure MSI-X table, PBA, masks
        Note over MSI: Configure MSI-X table, PBA, masks
        MSI->>SMNI: Response
        
    else TLB Configuration
        SMNI->>TLB: process_tlb_config(addr, data)
        Note over TLB: Set valid bits, masks, base addresses
        Note over TLB: TLB config via SMN works correctly
        TLB->>SMNI: Response
        
    else Unknown address
        SMNI->>SMN: TLM_ADDRESS_ERROR_RESPONSE
    end
```

**Configuration Targets:**

| Address Range [31:28] | Target | Functionality |
|----------------------|--------|---------------|
| 0x0 | Config Registers | system_ready, enables, isolation |
| 0x1 | SII Block | CORE_CONTROL, CFG_MODIFIED, BUS_DEV_NUM |
| 0x2 | Reserved | - |
| 0x3-0x7 | TLB Config | Inbound/Outbound TLB entries |
| 0x8 | MSI Relay | MSI-X table, PBA, masks |

**Note:** TLB configuration via SMN now works correctly. Configuration writes to TLB entries are properly routed and processed with correct offsets.

### 3.6 MSI Interrupt Generation Flow

**Transaction Path: NOC MSI Input → MSI Relay → PCIe Controller (MSI Output)**

```{mermaid}
sequenceDiagram
    participant NOC as NOC (MSI Source)
    participant MSI as MSI Relay Unit
    participant Tile as PCIe Tile
    participant PCIE as PCIe Controller

    NOC->>MSI: Write to msi_input (0x18800000)
    Note over MSI: process_msi_input() called
    Note over MSI: process_msi_input() called
    
    MSI->>MSI: Extract vector number
    MSI->>MSI: Check per-vector mask
    MSI->>MSI: Check global mask
    MSI->>MSI: Set PBA bit
    MSI->>MSI: Increment msi_outstanding
    MSI->>MSI: Trigger process_pending_msis()
    
    Note over Tile: signal_update_process()
    Tile->>MSI: Call process_pending_msis()
    
    alt MSI-X enabled AND not masked AND PBA set
        MSI->>MSI: Read MSI-X table entry
        MSI->>MSI: Prepare MSI transaction
        MSI->>PCIE: b_transport(msi_addr, msi_data)
        Note over PCIE: Forwarded via initiator socket<br/>to sparse_backing_memory
        PCIE->>MSI: TLM_OK_RESPONSE
        MSI->>MSI: Clear PBA bit
        MSI->>MSI: Decrement msi_outstanding
    end
```

**MSI Flow Steps:**

1. **MSI Input Write**
   - NOC writes to `msi_input_target` socket
   - Vector number encoded in write data

2. **MSI Relay Processing**
   - Check per-vector mask bit
   - Check global mask (`msix_mask_all_`)
   - Set corresponding PBA (Pending Bit Array) bit
   - Increment `msi_outstanding` counter

3. **MSI Generation** (via `process_pending_msis()`)
   - Called by tile's `signal_update_process()`
   - Check MSI-X enabled (`msix_enable_`)
   - For each pending PBA bit:
     - Read MSI-X table entry (address, data, mask)
     - If not masked, generate MSI transaction to PCIe
     - Clear PBA bit on successful delivery

**Note:** MSI output transactions are forwarded through the `pcie_controller_initiator` socket (now a real `tlm_utils::simple_initiator_socket<64>`) to the testbench's `sparse_backing_memory`, enabling cross-socket data verification of MSI delivery.

### 3.7 CII Tracking and Configuration Update

**Configuration Intercept Interface (CII) Flow:**

```{mermaid}
sequenceDiagram
    participant PCIE as PCIe Controller
    participant Tile as PCIe Tile
    participant SII as SII Block
    participant SMN as SMN Host

    Note over PCIE: PCIe config space write occurs
    PCIE->>Tile: Assert pcie_cii_hv signal
    PCIE->>Tile: Set pcie_cii_addr[7:0]
    PCIE->>Tile: Set pcie_cii_type[3:0]
    
    Note over Tile: signal_update_process()
    Tile->>SII: update()
    
    Note over SII: Read CII input signals
    alt pcie_cii_hv == 1
        SII->>SII: Check type == 0x04 (config write)
        alt type == 0x04
            SII->>SII: Check addr < 0x80 (first 128B)
            alt addr < 0x80
                SII->>SII: reg_index = addr[6:2]
                SII->>SII: cfg_modified_[reg_index] = 1
                SII->>SII: Assert config_update interrupt
                SII->>Tile: config_update = 1
                Tile->>SMN: Interrupt signaled
                
                Note over SMN: Software handles interrupt
                SMN->>SII: Read CFG_MODIFIED register
                SII->>SMN: Return modified bitmask
                SMN->>SII: Write to CFG_MODIFIED (RW1C)
                SII->>SII: Clear specified bits
                alt All bits cleared
                    SII->>SII: Deassert config_update
                    SII->>Tile: config_update = 0
                end
            end
        end
    end
    
    Note over Tile: Reset clears all state
    alt pcie_controller_reset_n == 0
        SII->>SII: cfg_modified_ = 0
        SII->>SII: config_update = 0
    end
```

**CII Tracking Details:**

1. **Detection Phase**
   - PCIe controller asserts `pcie_cii_hv` (handshake valid)
   - Provides address (`pcie_cii_addr`) and type (`pcie_cii_type`)
   - Type 0x04 indicates configuration write

2. **Filtering**
   - Only track first 128 bytes (addr < 0x80)
   - Covers standard PCIe config space registers
   - Calculate register index: `addr[6:2]` (32-bit aligned)

3. **State Update**
   - Set corresponding bit in 32-bit `cfg_modified_` bitmask
   - Assert `config_update` output signal (sticky)
   - Interrupt remains asserted until software clears

4. **Software Clear** (RW1C - Read/Write 1 to Clear)
   - SMN reads `CFG_MODIFIED` register to see which registers changed
   - SMN writes 1s to clear specific bits
   - When all bits cleared, `config_update` deasserts

5. **Reset Behavior**
   - Controller reset (`pcie_controller_reset_n=0`) clears all state
   - Both `cfg_modified_` and `config_update` reset to 0

### 3.8 Isolation and Recovery

**Isolation Sequence:**

```{mermaid}
sequenceDiagram
    participant EXT as External Control
    participant Tile as PCIe Tile
    participant CFG as Config Registers
    participant Switch as NOC-PCIE Switch
    participant PCIE as PCIe Controller

    Note over Tile: Normal Operation
    EXT->>Tile: Assert isolate_req signal
    
    Note over Tile: signal_update_process()
    Tile->>CFG: set_isolate_req(true)
    CFG->>CFG: system_ready = 0
    CFG->>CFG: pcie_inbound_app_enable = 0
    CFG->>CFG: pcie_outbound_app_enable = 0
    CFG->>Switch: Update enable signals
    
    Note over Switch: All PCIe transactions blocked
    PCIE->>Switch: b_transport(addr)
    Switch->>PCIE: TLM_ADDRESS_ERROR_RESPONSE
    
    Note over Tile: Attempt Recovery
    EXT->>Tile: Deassert isolate_req signal
    Tile->>CFG: set_isolate_req(false)
    CFG->>CFG: isolate_req_ = 0
    Note over CFG: Enables NOT restored
    
    Note over Switch: Still blocked (limitation)
    PCIE->>Switch: b_transport(addr)
    Switch->>PCIE: TLM_ADDRESS_ERROR_RESPONSE
    
    Note over Tile: Full Recovery Sequence
    EXT->>Tile: Cold reset cycle
    Note over Tile: Reset does NOT restore enables
    EXT->>CFG: SMN write system_ready=1
    EXT->>CFG: SMN write pcie_inbound_app_enable=1
    EXT->>CFG: SMN write pcie_outbound_app_enable=1
    Note over Switch: Normal operation restored
```

**Isolation Behavior:**

- **Trigger**: `isolate_req` signal assertion
- **Effect**: Clears all enable flags in `ConfigRegBlock`
- **Data Path**: All PCIe traffic returns DECERR
- **Critical Limitation**: `set_isolate_req(false)` does NOT restore enables
- **Recovery Requires**: Cold reset + full SMN reconfiguration

This is a known architectural limitation documented in the test plan.

### 3.9 Error Handling

**Error Conditions and Responses:**

| Condition | Detection Point | Response | Recovery |
|-----------|----------------|----------|----------|
| **Invalid TLB Entry** | TLB translate | `TLM_ADDRESS_ERROR_RESPONSE` | Configure valid TLB entry |
| **System Not Ready** | NOC-PCIE Switch | `TLM_ADDRESS_ERROR_RESPONSE` | Write `system_ready=1` |
| **Inbound Path Disabled** | NOC-PCIE Switch | `TLM_ADDRESS_ERROR_RESPONSE` | Write `pcie_inbound_app_enable=1` |
| **Outbound Path Disabled** | NOC-PCIE Switch | `TLM_ADDRESS_ERROR_RESPONSE` | Write `pcie_outbound_app_enable=1` |
| **Unknown Route** | NOC-PCIE Switch | `TLM_ADDRESS_ERROR_RESPONSE` | Use valid route (0x0,0x1,0x4,0x6,0x7,0xE,0xF) |
| **Unmapped SMN Address** | SMN-IO Switch | `TLM_ADDRESS_ERROR_RESPONSE` | Use valid SMN address range |
| **Bad TLM Command** | Switch routing | `TLM_COMMAND_ERROR_RESPONSE` | Use READ or WRITE command |
| **Isolation Active** | Config Registers | All paths blocked | Full recovery sequence required |
| **Page Boundary Cross** | TLB indexing | May access invalid entry | Align transactions to page boundaries |

**Error Response Flow:**

All error responses propagate back through the transaction chain:
- Switch/TLB detects error
- Sets `trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE)` or `TLM_COMMAND_ERROR_RESPONSE`
- Returns to initiator
- No retry mechanism (single-shot transactions)
- Software must detect and handle errors

### 3.10 Operating Mode Summary

| Mode | system_ready | inbound_enable | outbound_enable | isolate_req | Data Flow | Use Case |
|------|--------------|----------------|-----------------|-------------|-----------|----------|
| **Normal** | 1 | 1 | 1 | 0 | Full bidirectional | Standard operation |
| **System Only** | 1 | 0 | 0 | 0 | System paths only | Pre-application init |
| **Inbound Only** | 1 | 1 | 0 | 0 | PCIe→NOC only | Receive-only mode |
| **Outbound Only** | 1 | 0 | 1 | 0 | NOC→PCIe only | Transmit-only mode |
| **Bypass** | 0 | x | x | 0 | Status register only | Pre-init diagnostics |
| **Isolated** | 0 | 0 | 0 | 1 | All blocked | Fault containment |
| **Reset** | 0 | 0 | 0 | 0 | All blocked | Coming out of reset |

**Mode Transitions:**

- **Reset → Normal**: Requires software configuration sequence
- **Normal → Isolated**: Automatic on `isolate_req` assertion
- **Isolated → Normal**: Requires full recovery (reset + reconfig)
- **Normal ↔ System Only**: Software enable/disable of application paths
- **Any → Reset**: Hardware signal (`cold_reset_n`, `warm_reset_n`)

---

## 4. Detailed Block Diagram

### 3.1 Complete System Architecture

```{mermaid}
graph TB
    subgraph "NOC Domain"
        NOCN[NOC Network]
    end
    
    subgraph "SMN Domain"
        SMNN[SMN Network]
    end
    
    subgraph "PCIe Domain"
        PCTRL[PCIe Controller]
    end
    
    subgraph "Keraunos PCIe Tile"
        subgraph "NOC-PCIE Switch"
            NOCP1[Route Decoder<br/>bits 63:60]
            NOCP2[Status Register<br/>routes 0xE/0xF]
            NOCP3[Isolation Gate]
        end
        
        subgraph "Inbound TLBs"
            TLBS[TLB Sys In0<br/>16KB pages]
            TLBA0[TLB App In0 instances 0-3<br/>16MB pages]
            TLBA1[TLB App In1<br/>8GB pages]
        end
        
        subgraph "Outbound TLBs"
            TLBSO[TLB Sys Out0<br/>16KB pages]
            TLBAO0[TLB App Out0<br/>16MB pages]
            TLBAO1[TLB App Out1<br/>8GB pages]
        end
        
        subgraph "NOC-IO Switch"
            NOCI1[NOC Output]
            NOCI2[MSI Input Route]
            NOCI3[TLB App Out Route]
        end
        
        subgraph "SMN-IO Switch"
            SMNI1[Config Router<br/>0x18xxxxxx]
            SMNI2[TLB Config<br/>8 instances]
            SMNI3[Sys TLB Bypass]
        end
        
        subgraph "Control & Status"
            CFG1[Config Registers<br/>0x18000000]
            SII1[SII Block<br/>0x18101000<br/>CII Tracking]
            MSI1[MSI Relay<br/>0x18800000<br/>16 vectors]
            PHY1[PCIe PHY<br/>0x18102000/3000]
        end
        
        subgraph "Clock/Reset"
            CLK1[Cold Reset]
            CLK2[Warm Reset]
            CLK3[Isolation Control]
        end
    end
    
    NOCN <-->|noc_n_target<br/>noc_n_initiator| NOCI1
    SMNN <-->|smn_n_target<br/>smn_n_initiator| SMNI1
    PCTRL <-->|pcie_controller_target<br/>pcie_controller_initiator| NOCP1
    
    NOCP1 --> TLBS
    NOCP1 --> TLBA0
    NOCP1 --> TLBA1
    TLBS --> NOCI1
    TLBA0 --> NOCI1
    TLBA1 --> NOCI1
    
    NOCI1 --> TLBSO
    NOCI1 --> TLBAO0
    NOCI1 --> TLBAO1
    TLBSO --> NOCP1
    TLBAO0 --> NOCP1
    TLBAO1 --> NOCP1
    
    NOCI2 --> MSI1
    MSI1 --> NOCP1
    
    SMNI1 --> CFG1
    SMNI1 --> SII1
    SMNI1 --> MSI1
    SMNI1 --> SMNI2
    SMNI2 -.->|Config| TLBS
    SMNI2 -.->|Config| TLBA0
    SMNI2 -.->|Config| TLBA1
    SMNI2 -.->|Config| TLBSO
    SMNI2 -.->|Config| TLBAO0
    SMNI2 -.->|Config| TLBAO1
    SMNI1 --> PHY1
    
    PCTRL -.->|CII signals| SII1
    
    CLK1 -.->|Control| NOCP3
    CLK2 -.->|Control| CFG1
    CLK3 -.->|Control| NOCP3
    
    style NOCP1 fill:#e1f5ff
    style NOCI1 fill:#e1f5ff
    style SMNI1 fill:#e1f5ff
    style TLBS fill:#fff4e1
    style TLBA0 fill:#fff4e1
    style TLBA1 fill:#fff4e1
    style TLBSO fill:#fff4e1
    style TLBAO0 fill:#fff4e1
    style TLBAO1 fill:#fff4e1
    style MSI1 fill:#ffe1f5
    style SII1 fill:#ffe1f5
    style CFG1 fill:#e1ffe1
    style CLK1 fill:#f5e1e1
    style CLK2 fill:#f5e1e1
    style CLK3 fill:#f5e1e1
```

---

## 5. Data Flow Paths

### 4.1 Inbound Data Flow (PCIe → NOC)

```{mermaid}
sequenceDiagram
    participant PC as PCIe Controller
    participant NPS as NOC-PCIE Switch
    participant TLB as Inbound TLB
    participant NIS as NOC-IO Switch
    participant NOC as NOC Network
    
    PC->>NPS: Write(addr=0x1xxx, data)
    Note over NPS: Route decode: bits[63:60]=0x1
    NPS->>NPS: Check isolation/enables
    NPS->>TLB: TLB App In1
    Note over TLB: Index = (addr>>33) & 0x3F = 0
    Note over TLB: Valid check: entry[0].valid = true
    TLB->>TLB: Translate address
    Note over TLB: translated = (entry[0].addr<<12) | offset
    TLB->>NIS: Forward with translated addr
    NIS->>NOC: Route via noc_n_initiator socket
    Note over NOC: Forwarded to sparse_backing_memory<br/>for cross-socket data verification
    NOC-->>NIS: TLM_OK_RESPONSE
    NIS-->>TLB: Response
    TLB-->>NPS: Response
    NPS-->>PC: TLM_OK_RESPONSE
```

### 4.2 Outbound Data Flow (NOC → PCIe)

```{mermaid}
sequenceDiagram
    participant NOC as NOC Network
    participant NIS as NOC-IO Switch
    participant TLB as Outbound TLB
    participant NPS as NOC-PCIE Switch
    participant PC as PCIe Controller
    
    NOC->>NIS: Read(addr=0x10001xxx)
    Note over NIS: Route to TLB App Out0
    NIS->>TLB: Forward transaction
    Note over TLB: Index = (addr>>24) & 0x3F
    TLB->>TLB: Translate address
    Note over TLB: Check entry valid, translate
    TLB->>NPS: Forward to PCIe
    NPS->>PC: TLM transaction via initiator socket
    Note over PC: Forwarded to sparse_backing_memory<br/>for cross-socket data verification
    PC-->>NPS: Data response
    NPS-->>TLB: Response
    TLB-->>NIS: Response
    NIS-->>NOC: TLM_OK_RESPONSE + data
```

### 4.3 Configuration Access Flow (SMN → Config)

```{mermaid}
sequenceDiagram
    participant SMN as SMN Network
    participant SIS as SMN-IO Switch
    participant CFG as Config Target
    
    SMN->>SIS: Write(addr=0x18101004, data)
    Note over SIS: Route decode: base check
    alt Address = 0x18000000-0x18000FFF
        SIS->>CFG: Config Reg Block
        Note over CFG: System control registers
    else Address = 0x18101000-0x1811xxxx
        SIS->>CFG: SII Config (APB)
        Note over CFG: CII config registers
    else Address = 0x18200000-0x182Fxxxx
        SIS->>CFG: TLB Config (8 instances)
        Note over CFG: TLB entry configuration
    else Address = 0x18800000-0x18800FFF
        SIS->>CFG: MSI Relay Config
        Note over CFG: MSI-X table, PBA, mask
    end
    
    CFG-->>SIS: Response (may be DECERR)
    Note over CFG: TLB config via SMN<br/>works correctly
    SIS-->>SMN: Response
```

### 4.4 MSI Generation Flow

```{mermaid}
sequenceDiagram
    participant NOC as NOC Network
    participant NIS as NOC-IO Switch
    participant MSI as MSI Relay
    participant NPS as NOC-PCIE Switch
    participant PC as PCIe Controller
    
    NOC->>NIS: Write(addr=0x18800000, data=vector)
    Note over NIS: Detect MSI input address
    NIS->>MSI: process_msi_input()
    Note over MSI: Check: msix_enable &&<br/>!global_mask &&<br/>!vector_mask[vector]
    
    alt MSI enabled and unmasked
        MSI->>MSI: Read MSI-X table[vector]
        Note over MSI: Get address, data from table
        MSI->>MSI: Increment outstanding counter
        MSI->>NPS: Generate MSI transaction
        NPS->>PC: MSI write(table_addr, table_data)
        PC-->>NPS: MSI delivered
        NPS-->>MSI: Complete
        MSI->>MSI: Decrement outstanding
    else MSI masked or disabled
        Note over MSI: PBA bit set for later delivery
    end
    
    MSI-->>NIS: TLM_OK_RESPONSE
    NIS-->>NOC: Complete
```

### 4.5 CII Interrupt Flow

```{mermaid}
sequenceDiagram
    participant PC as PCIe Controller
    participant CII as CII Signals
    participant SII as SII Block
    participant INT as Interrupt Output
    
    Note over PC: Host writes to<br/>PCIe config space
    PC->>CII: pcie_cii_hv = true
    PC->>CII: pcie_cii_hdr_type = 0x04
    PC->>CII: pcie_cii_hdr_addr = 0x010
    
    Note over CII: Signal propagation<br/>wait(SC_ZERO_TIME)
    
    CII->>SII: signal_update_process()
    Note over SII: SiiBlock::update() called
    
    SII->>SII: CII Tracking Phase
    Note over SII: Check: hv && type==0x04 &&<br/>addr<0x080
    SII->>SII: reg_index = addr >> 2 = 4
    SII->>SII: cii_new_bits = (1 << 4)
    
    SII->>SII: cfg_modified Update
    Note over SII: cfg_modified |= cii_new_bits
    
    SII->>SII: Interrupt Generation
    Note over SII: config_int = (cfg_modified != 0)
    
    SII->>INT: config_update = true
    Note over INT: Interrupt asserted
    
    Note over PC: Later: reset pulse or RW1C
    PC->>SII: pcie_controller_reset_n = 0→1
    SII->>SII: Clear cfg_modified
    SII->>INT: config_update = false
```

---

## 6. Module Descriptions

### 5.1 NOC-PCIE Switch

**Purpose:** Routes traffic from PCIe controller to appropriate destination (TLBs, bypass paths, status register).

**Route Table:**

| Route [63:60] | Destination | Description |
|--------------|-------------|-------------|
| 0x0 | TLB App In0 [0] | Application TLB instance 0 |
| 0x1 | TLB App In1 | Application TLB (large pages) |
| 0x4 | TLB Sys In0 | System TLB |
| 0x8 | Bypass App | Direct to NOC-IO |
| 0x9 | Bypass Sys | Direct to SMN-IO |
| 0xE, 0xF | Status Register | Read-only status (if system_ready) |
| Other | DECERR | TLM_ADDRESS_ERROR_RESPONSE |

**Status Register Format:**
```
[0]    system_ready
[1]    pcie_outbound_app_enable
[2]    pcie_inbound_app_enable
[31:3] Reserved (0)
```

### 5.2 Translation Lookaside Buffers

**TLB Configuration:**

```{mermaid}
graph LR
    subgraph "Inbound TLBs - 5 instances"
        TLBS[TLB Sys In0<br/>16KB pages<br/>shift=14, index 19:14]
        TLBA0[TLB App In0 instances 0-3<br/>16MB pages<br/>shift=24, index 29:24]
        TLBA1[TLB App In1<br/>8GB pages<br/>shift=33, index 38:33]
    end
    
    subgraph "Outbound TLBs - 3 instances"
        TLBSO[TLB Sys Out0<br/>16KB pages<br/>shift=14, index 19:14]
        TLBAO[TLB App Out0<br/>16MB pages<br/>shift=24, index 29:24]
        TLBAO1[TLB App Out1<br/>8GB pages<br/>shift=33, index 38:33]
    end
    
    subgraph "TLB Entry Structure"
        ENT[valid: 1 bit<br/>addr: 52 bits 63:12<br/>attr: 32 bits AxUSER]
    end
    
    style TLBS fill:#fff4e1
    style TLBA0 fill:#fff4e1
    style TLBA1 fill:#fff4e1
    style TLBSO fill:#fff4e1
    style TLBAO fill:#fff4e1
    style TLBAO1 fill:#fff4e1
    style ENT fill:#e1ffe1
```

**Translation Algorithm:**
1. `index = (address >> page_shift) & 0x3F`
2. If `index >= 64` or `!entries[index].valid` → DECERR
3. `translated_addr = (entries[index].addr << 12) | (address & page_mask)`
4. `axuser = entries[index].attr`

### 5.3 MSI Relay Unit

**Architecture:**

```{mermaid}
graph TB
    subgraph "MSI Relay Components"
        TBL[MSI-X Table<br/>16 entries<br/>address, data, mask]
        PBA[Pending Bit Array<br/>16 bits<br/>read-only]
        CTL[Control<br/>msix_enable<br/>msix_mask<br/>outstanding counter]
    end
    
    subgraph "Inputs"
        NOCI[NOC-IO Switch<br/>MSI input @ 0x18800000]
        SMNI[SMN-IO Switch<br/>Config @ 0x18800000]
    end
    
    subgraph "Output"
        NOCP[NOC-PCIE Switch<br/>MSI to PCIe]
    end
    
    NOCI -->|process_msi_input| CTL
    SMNI -.->|config access<br/>via SMN| TBL
    CTL --> TBL
    CTL --> PBA
    TBL -->|MSI generation| NOCP
    
    style TBL fill:#ffe1f5
    style PBA fill:#ffe1f5
    style CTL fill:#ffe1f5
```

**MSI-X Table Entry (16 bytes):**
```
[63:0]   Message Address (64-bit)
[95:64]  Message Data (32-bit)
[96]     Per-Vector Mask (1=masked)
[127:97] Reserved
```

### 5.4 System Information Interface (SII)

**CII Tracking State Machine:**

```{mermaid}
stateDiagram-v2
    [*] --> Idle: Reset
    
    Idle --> WaitCII: Normal operation
    WaitCII --> DetectWrite: CII handshake valid
    
    DetectWrite --> CheckType: Read CII header
    CheckType --> CheckAddr: Config write detected
    CheckType --> WaitCII: Not config write
    
    CheckAddr --> SetBit: First 128B of config space
    CheckAddr --> WaitCII: Beyond 128B boundary
    
    SetBit --> AssertInt: Set modified bit
    AssertInt --> WaitCII: Assert interrupt
    
    WaitCII --> ClearInt: Controller reset
    ClearInt --> Idle: Clear all state
    
    note right of SetBit: Tracks 32 config registers
    note right of AssertInt: Sticky until reset or RW1C
```

**APB Registers:**

| Offset | Register | Access | Description |
|--------|----------|--------|-------------|
| 0x0000 | CORE_CONTROL | RW | Device type [2:0]: 0x4=RP |
| 0x0004 | CFG_MODIFIED | RW1C | Config modified bitmask (32 bits) |
| 0x0008 | BUS_DEV_NUM | RW | Bus[15:8], Device[7:0] numbers |

---

## 7. Address Map

### 6.1 Inbound Address Routing

```{mermaid}
graph TD
    START[PCIe Transaction<br/>addr 63:0]
    
    START --> ROUTE{Route bits<br/>63:60}
    
    ROUTE -->|0x0| TLB0[TLB App In0 inst 0<br/>16MB pages]
    ROUTE -->|0x1| TLB1[TLB App In1<br/>8GB pages]
    ROUTE -->|0x4| TLB4[TLB Sys In0<br/>16KB pages]
    ROUTE -->|0x8| BP8[Bypass App<br/>NOC-IO direct]
    ROUTE -->|0x9| BP9[Bypass Sys<br/>SMN-IO direct]
    ROUTE -->|0xE,0xF| STAT[Status Register<br/>Read-only]
    ROUTE -->|Other| ERR[DECERR<br/>TLM_ADDRESS_ERROR]
    
    TLB0 --> NOC[NOC Network]
    TLB1 --> NOC
    TLB4 --> NOC
    BP8 --> NOC
    BP9 --> SMN[SMN Network]
    STAT --> RET[Return status word]
    
    style TLB0 fill:#fff4e1
    style TLB1 fill:#fff4e1
    style TLB4 fill:#fff4e1
    style BP8 fill:#e1f5ff
    style BP9 fill:#e1f5ff
    style STAT fill:#e1ffe1
    style ERR fill:#ffe1e1
```

### 6.2 Configuration Address Map

**SMN Configuration Space (base 0x18000000):**

| Base Address | Size | Module | Purpose |
|--------------|------|--------|---------|
| 0x18000000 | 4KB | Config Reg Block | System control, enables |
| 0x18101000 | 64KB | SII APB | Device type, CFG_MODIFIED, bus/dev |
| 0x18102000 | 4KB | PCIe PHY APB | PHY APB registers |
| 0x18103000 | 4KB | PCIe PHY AHB | PHY AHB registers |
| 0x18200000 | 4KB | TLB Sys In0 | TLB entry config (64×16B) |
| 0x18210000 | 4KB | TLB App In0 [0] | TLB entry config |
| 0x18220000 | 4KB | TLB App In0 [1] | TLB entry config |
| 0x18230000 | 4KB | TLB App In0 [2] | TLB entry config |
| 0x18240000 | 4KB | TLB App In0 [3] | TLB entry config |
| 0x18250000 | 4KB | TLB App In1 | TLB entry config |
| 0x18260000 | 4KB | TLB Sys Out0 | TLB entry config |
| 0x18270000 | 4KB | TLB App Out0 | TLB entry config |
| 0x18280000 | 4KB | TLB App Out1 | TLB entry config |
| 0x18800000 | 4KB | MSI Relay | MSI-X table, PBA, mask |

---

## 8. Clock and Reset Strategy

### 7.1 Reset Hierarchy

```{mermaid}
graph TD
    subgraph "Reset Sources"
        COLD[cold_reset_n<br/>Top-level reset]
        WARM[warm_reset_n<br/>Warm reset]
        PCIER[pcie_controller_reset_n<br/>PCIe-specific]
        ISOL[isolate_req<br/>Isolation control]
    end
    
    subgraph "Reset Targets"
        CLK[Clock/Reset Control<br/>pcie_sii_reset_ctrl<br/>pcie_reset_ctrl]
        SII[SII Block<br/>cfg_modified<br/>config_update]
        CFG[Config Registers<br/>system_ready<br/>enables]
    end
    
    COLD --> CLK
    WARM --> CLK
    PCIER --> SII
    ISOL --> CFG
    
    style COLD fill:#f5e1e1
    style WARM fill:#f5e1e1
    style PCIER fill:#f5e1e1
    style ISOL fill:#fff4e1
    style CLK fill:#e1ffe1
    style SII fill:#ffe1f5
    style CFG fill:#e1ffe1
```

### 7.2 Reset Effects

| Reset Signal | Affects | Preserves |
|--------------|---------|-----------|
| `cold_reset_n` | Clock/Reset control registers | Config, TLBs, enables, system_ready |
| `warm_reset_n` | Same as cold reset | Same as cold reset |
| `pcie_controller_reset_n` | SII cfg_modified, config_update | All other state |
| `isolate_req` | Clears all enable flags | TLB config, register values |

### 7.3 Isolation Sequence

```{mermaid}
sequenceDiagram
    participant EXT as External Control
    participant CFG as Config Reg Block
    participant SW as NOC-PCIE Switch
    
    Note over CFG: Initial State:<br/>system_ready=1<br/>inbound_enable=1<br/>outbound_enable=1
    
    EXT->>CFG: isolate_req = 1
    Note over CFG: wait(SC_ZERO_TIME)
    CFG->>CFG: system_ready = 0
    CFG->>CFG: inbound_enable = 0
    CFG->>CFG: outbound_enable = 0
    CFG->>SW: Update enables
    Note over SW: All PCIe traffic → DECERR
    
    Note over EXT: Later: attempt recovery
    EXT->>CFG: isolate_req = 0
    Note over CFG: wait(SC_ZERO_TIME)
    Note over CFG: Enables NOT restored!<br/>Permanent state loss
    
    Note over SW: Traffic still blocked<br/>No recovery mechanism
```

---

## 9. Interface Specifications

### 8.1 External TLM Sockets

| Socket Name | Type | Width | Direction | Description |
|-------------|------|-------|-----------|-------------|
| `noc_n_target` | Target | 64-bit | Inbound | NOC → Tile (outbound) |
| `noc_n_initiator` | Initiator | 64-bit | Outbound | Tile → NOC (inbound), forwards to `sparse_backing_memory` for cross-socket data verification |
| `smn_n_target` | Target | 64-bit | Inbound | SMN → Tile (config) |
| `smn_n_initiator` | Initiator | 64-bit | Outbound | Tile → SMN (bypass), forwards to `sparse_backing_memory` for cross-socket data verification |
| `pcie_controller_target` | Target | 64-bit | Inbound | PCIe → Tile |
| `pcie_controller_initiator` | Initiator | 64-bit | Outbound | Tile → PCIe, forwards to `sparse_backing_memory` for cross-socket data verification |

**Note:** The three "initiator" sockets (`noc_n_initiator`, `smn_n_initiator`, `pcie_controller_initiator`) are implemented as `tlm_utils::simple_initiator_socket<64>` types that forward transactions to the testbench's `sparse_backing_memory` for cross-socket data verification. This enables end-to-end data integrity checks where data written through one socket can be read back through another.

### 8.2 Control and Status Signals

**Input Signals:**

| Signal Name | Width | Description |
|-------------|-------|-------------|
| `cold_reset_n` | 1 | Cold reset (active low) |
| `warm_reset_n` | 1 | Warm reset (active low) |
| `isolate_req` | 1 | Isolation request |
| `pcie_cii_hv` | 1 | CII header valid |
| `pcie_cii_hdr_type` | 5 | CII header type |
| `pcie_cii_hdr_addr` | 12 | CII header address |
| `pcie_core_clk` | 1 | PCIe core clock |
| `pcie_controller_reset_n` | 1 | PCIe controller reset |
| `pcie_flr_request` | 1 | Function-level reset request |
| `pcie_hot_reset` | 1 | Hot reset indication |
| `pcie_ras_error` | 1 | RAS error indication |
| `pcie_dma_completion` | 1 | DMA completion |
| `pcie_misc_int` | 1 | Miscellaneous interrupt |
| `axi_clk` | 1 | AXI clock |

**Output Signals:**

| Signal Name | Width | Description |
|-------------|-------|-------------|
| `pcie_app_bus_num` | 8 | Application bus number |
| `pcie_app_dev_num` | 8 | Application device number |
| `pcie_device_type` | 1 | 0=EP, 1=RP |
| `pcie_sys_int` | 1 | System interrupt |
| `function_level_reset` | 1 | FLR forwarded |
| `hot_reset_requested` | 1 | Hot reset forwarded |
| `config_update` | 1 | Config update interrupt (CII) |
| `ras_error` | 1 | RAS error forwarded |
| `dma_completion` | 1 | DMA completion forwarded |
| `controller_misc_int` | 1 | Misc interrupt forwarded |
| `noc_timeout` | 3 | NOC timeout status |

---

## 10. Known Limitations and Findings

### 9.1 Architecture Limitations

```{mermaid}
graph TB
    subgraph "Known Issues"
        ISOL[Isolation Recovery<br/>No restore mechanism]
    end
    
    subgraph "Resolved Issues"
        PASS[Address Passthrough<br/>RESOLVED - TLB config<br/>via SMN works correctly]
        MSI[MSI PBA<br/>RESOLVED - initiator<br/>sockets to backing memory]
    end
    
    subgraph "Impacts"
        REC[Isolation<br/>Single-use only]
    end
    
    subgraph "Mitigations"
        AVOID[Avoid Isolation<br/>in test env]
        BACK[sparse_backing_memory<br/>Cross-socket verification]
    end
    
    ISOL --> REC
    
    REC -.->|Mitigation| AVOID
    PASS -.->|Fixed| BACK
    MSI -.->|Fixed| BACK
    
    style PASS fill:#e1ffe1
    style ISOL fill:#ffe1e1
    style MSI fill:#e1ffe1
    style REC fill:#fff4e1
    style AVOID fill:#e1ffe1
    style BACK fill:#e1ffe1
```

### 9.2 Documented Findings

1. **Address Passthrough (RESOLVED):** SMN-IO/NOC-IO switches now correctly route TLB configuration via SMN. TLB config writes work correctly.
2. **TLB Configuration (RESOLVED):** TLB config writes via SMN now function correctly, enabling proper address translation setup.
3. **MSI Relay (RESOLVED):** MSI output transactions forwarded through real initiator sockets to `sparse_backing_memory` for cross-socket data verification.
4. **Signal Propagation (RESOLVED):** `sc_core::wait(SC_ZERO_TIME)` required for signal changes to propagate.
5. **Isolation Recovery Impossible:** `isolate_req=false` does not restore enables. This is a known architectural limitation.
6. **CII Processing (RESOLVED):** Restored via `SiiBlock::update()` method.
7. **DUT Initiator Socket Architecture (NEW):** All three initiator sockets (`noc_n_initiator`, `smn_n_initiator`, `pcie_controller_initiator`) are now `tlm_utils::simple_initiator_socket<64>` types that forward to `sparse_backing_memory`, enabling cross-socket data verification.

---

## 11. Test Coverage Summary

### 10.1 Test Suite Overview

**81 test cases, 289 assertions, 0 failures**

```{mermaid}
pie title Test Distribution
    "E2E Tests (41)" : 41
    "Directed Tests (40)" : 40
```

```{mermaid}
graph LR
    subgraph "Test Categories"
        E2E[E2E Tests<br/>41 tests]
        DIR[Directed Tests<br/>40 tests]
    end
    
    subgraph "E2E Breakdown"
        E1[Inbound TLB: 10]
        E2[Outbound TLB: 9]
        E3[Bypass: 2]
        E4[MSI: 8]
        E5[SII: 4]
        E6[Switch: 4]
        E7[Power/Isolation: 2]
        E8[Integration: 2]
    end
    
    subgraph "Directed Breakdown"
        D1[TLB Unit: 8]
        D2[MSI Unit: 5]
        D3[Switch Unit: 7]
        D4[Config: 3]
        D5[SII: 6]
        D6[Signals: 3]
        D7[Reset: 2]
        D8[Integration: 1]
        D9[Data Verification: 5]
    end
    
    E2E --> E1
    E2E --> E2
    E2E --> E3
    E2E --> E4
    E2E --> E5
    E2E --> E6
    E2E --> E7
    E2E --> E8
    
    DIR --> D1
    DIR --> D2
    DIR --> D3
    DIR --> D4
    DIR --> D5
    DIR --> D6
    DIR --> D7
    DIR --> D8
    DIR --> D9
    
    style E2E fill:#e1f5ff
    style DIR fill:#fff4e1
```

### 10.2 Coverage Metrics

- **Functional Coverage:** 100% of testable features
- **Route Coverage:** All 16 route values exercised (0x0-0xF)
- **TLB Boundary Coverage:** Entry 0, 1, 63 tested
- **Error Path Coverage:** DECERR, isolation blocking, invalid TLB entries
- **Signal Coverage:** All input/output signals exercised
- **Cross-Socket Data Verification:** Enabled via `sparse_backing_memory` through real initiator sockets

---

## 12. References

### 11.1 Related Documentation

- **SystemC Design Document:** Detailed low-level implementation
- **Test Plan:** Comprehensive verification strategy and results
- **Test Implementation:** 81 test cases in `Keranous_pcie_tileTest.cc`

### 11.2 Source Code Organization

```
Keraunos_PCIe_tile/
├── SystemC/
│   ├── include/         (15 header files)
│   └── src/             (15 implementation files)
├── Tests/
│   └── Unittests/
│       ├── Keranous_pcie_tileTest.cc        (81 tests)
│       └── Keranous_pcie_tileTestHarness.h
└── doc/
    ├── Keraunos_PCIe_Tile_HLD.md
    ├── Keraunos_PCIE_Tile_SystemC_Design_Document.md
    └── Keraunos_PCIE_Tile_Testplan.md
```

---

**Document Control:**
- **Version:** 2.0
- **Date:** February 10, 2026
- **Status:** Updated - DUT initiator socket architecture
- **Next Review:** Upon DUT architecture changes or test plan updates
