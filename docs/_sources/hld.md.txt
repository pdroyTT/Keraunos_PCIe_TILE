# Keraunos PCIe Tile - High-Level Design

**Version:** 1.0  
**Date:** February 5, 2026  
**Status:** Released  

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
   - Forward translated address to NOC
   - Preserve AxUSER field
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
   - Forward to PCIe controller if enabled

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
        Note over MSI: Address passthrough limits functionality
        MSI->>SMNI: Response
        
    else TLB Configuration
        SMNI->>TLB: process_tlb_config(addr, data)
        Note over TLB: Set valid bits, masks, base addresses
        Note over TLB: Address passthrough affects offset calculation
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

**Known Limitation:** SMN-IO switch passes full SystemC addresses to internal callbacks instead of stripping base addresses. This causes configuration writes to MSI, SII, and TLB to be processed with incorrect offsets.

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
    Note over MSI: Address passthrough: offset != 0
    
    alt offset == 0 (not met due to passthrough)
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
            PCIE->>MSI: TLM_OK_RESPONSE
            MSI->>MSI: Clear PBA bit
            MSI->>MSI: Decrement msi_outstanding
        end
        
    else offset != 0 (actual behavior)
        MSI->>NOC: TLM_OK_RESPONSE
        Note over MSI: PBA not updated (limitation)
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

**Current Limitation:** Address passthrough causes `process_msi_input` to receive full address 0x18800000 instead of offset 0, preventing PBA updates. MSI output path cannot be fully exercised in current test environment.

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
    NIS->>NOC: Route to NOC output
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
    NPS->>PC: TLM transaction
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
    Note over CFG: Address passthrough issue:<br/>receives full addr, expects offset
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
        Note over MSI: PBA bit should be set<br/>(blocked by address passthrough)
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
    SMNI -.->|config access<br/>blocked by passthrough| TBL
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
| `noc_n_initiator` | Target | 64-bit | Outbound | Tile → NOC (inbound) |
| `smn_n_target` | Target | 64-bit | Inbound | SMN → Tile (config) |
| `smn_n_initiator` | Target | 64-bit | Outbound | Tile → SMN (bypass) |
| `pcie_controller_target` | Target | 64-bit | Inbound | PCIe → Tile |
| `pcie_controller_initiator` | Target | 64-bit | Outbound | Tile → PCIe |

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
        PASS[Address Passthrough<br/>Full address vs offset]
        ISOL[Isolation Recovery<br/>No restore mechanism]
        MSI[MSI PBA<br/>Offset mismatch]
    end
    
    subgraph "Impacts"
        CFG[Config Writes<br/>Mostly blocked]
        REC[Isolation<br/>Single-use only]
        INT[MSI Interrupt<br/>PBA not set]
    end
    
    subgraph "Workarounds"
        DEF[Default Init<br/>Entry 0 valid]
        AVOID[Avoid Isolation<br/>in test env]
        RELAY[MSI Relay<br/>Still functional]
    end
    
    PASS --> CFG
    ISOL --> REC
    MSI --> INT
    
    CFG -.->|Mitigation| DEF
    REC -.->|Mitigation| AVOID
    INT -.->|Mitigation| RELAY
    
    style PASS fill:#ffe1e1
    style ISOL fill:#ffe1e1
    style MSI fill:#ffe1e1
    style CFG fill:#fff4e1
    style REC fill:#fff4e1
    style INT fill:#fff4e1
    style DEF fill:#e1ffe1
    style AVOID fill:#e1ffe1
    style RELAY fill:#e1ffe1
```

### 9.2 Documented Findings

1. **Address Passthrough:** SMN-IO/NOC-IO switches pass full addresses to callbacks instead of relative offsets → config writes largely non-functional
2. **TLB Configuration Blocked:** TLB config writes routed to data paths instead of config callbacks
3. **MSI Relay Configuration Limited:** MSI-X enable/mask are internal signals; config writes blocked by passthrough
4. **Signal Propagation (RESOLVED):** `sc_core::wait(SC_ZERO_TIME)` required for signal changes to propagate
5. **Isolation Recovery Impossible:** `isolate_req=false` does not restore enables
6. **CII Processing (RESOLVED):** Restored via `SiiBlock::update()` method
7. **MSI PBA Not Verifiable:** PBA bit set requires offset=0, but receives 0x18800000

---

## 11. Test Coverage Summary

### 10.1 Test Suite Overview

**76 test cases, 251 assertions, 0 failures**

```{mermaid}
pie title Test Distribution
    "E2E Tests (41)" : 41
    "Directed Tests (35)" : 35
```

```{mermaid}
graph LR
    subgraph "Test Categories"
        E2E[E2E Tests<br/>41 tests]
        DIR[Directed Tests<br/>35 tests]
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
    
    style E2E fill:#e1f5ff
    style DIR fill:#fff4e1
```

### 10.2 Coverage Metrics

- **Functional Coverage:** 100% of testable features
- **Route Coverage:** All 16 route values exercised (0x0-0xF)
- **TLB Boundary Coverage:** Entry 0, 1, 63 tested
- **Error Path Coverage:** DECERR, isolation blocking, invalid TLB entries
- **Signal Coverage:** All input/output signals exercised

---

## 12. References

### 11.1 Related Documentation

- **SystemC Design Document:** Detailed low-level implementation
- **Test Plan:** Comprehensive verification strategy and results
- **Test Implementation:** 76 test cases in `Keranous_pcie_tileTest.cc`

### 11.2 Source Code Organization

```
Keraunos_PCIe_tile/
├── SystemC/
│   ├── include/         (15 header files)
│   └── src/             (15 implementation files)
├── Tests/
│   └── Unittests/
│       ├── Keranous_pcie_tileTest.cc        (76 tests)
│       └── Keranous_pcie_tileTestHarness.h
└── doc/
    ├── Keraunos_PCIe_Tile_HLD.md
    ├── Keraunos_PCIE_Tile_SystemC_Design_Document.md
    └── Keraunos_PCIE_Tile_Testplan.md
```

---

**Document Control:**
- **Version:** 1.0
- **Date:** February 5, 2026
- **Status:** Released
- **Next Review:** Upon DUT architecture changes or test plan updates
