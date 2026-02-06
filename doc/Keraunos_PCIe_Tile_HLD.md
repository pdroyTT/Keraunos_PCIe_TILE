# Keraunos PCIe Tile - High-Level Design Document

**Version:** 1.0  
**Date:** February 5, 2026  
**Author:** Auto-generated from SystemC implementation and test coverage analysis  

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Architecture Overview](#2-architecture-overview)
3. [Block Diagram](#3-block-diagram)
4. [Module Descriptions](#4-module-descriptions)
5. [Interface Specifications](#5-interface-specifications)
6. [Address Map](#6-address-map)
7. [Data Flow](#7-data-flow)
8. [Clock and Reset Strategy](#8-clock-and-reset-strategy)
9. [Configuration and Control](#9-configuration-and-control)
10. [References](#10-references)

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

### 1.3 Definitions and Acronyms

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

```
┌─────────────────────────────────────────────────────────────────┐
│                    Keraunos PCIe Tile                           │
│                                                                 │
│  NOC-N ──────► NOC-PCIE ──┬──► Inbound TLBs ──► NOC-IO ──► NOC-N│
│  (Target/Init)  Switch    │    (5 instances)     Switch    (Out)│
│                           │                                     │
│                           └──► Outbound TLBs ◄── NOC-IO ◄────┘  │
│                                (3 instances)     Switch         │
│                                                                 │
│  SMN-N ──────► SMN-IO ────┬──► Config Registers                │
│  (Target/Init)  Switch    ├──► TLB Config (8x)                 │
│                           ├──► MSI Relay Config                │
│                           ├──► SII Config (APB)                │
│                           ├──► PCIe PHY Config                 │
│                           └──► Bypass (TLB Sys)                │
│                                                                 │
│  PCIe Ctrl ──► NOC-PCIE ──┬──► Inbound paths (via TLBs)        │
│  (Target/Init)  Switch    ├──► Bypass paths (NOC-IO/SMN-IO)    │
│                           └──► Status Register (route 0xE/0xF)  │
│                                                                 │
│  Clock/Reset Control ◄──── cold_reset_n, warm_reset_n          │
│  SII Block ◄──────────────── CII inputs (config intercept)     │
│  MSI Relay ◄──────────────── MSI input traffic (from NOC-IO)   │
│                                                                 │
│  Interrupt Outputs: config_update, FLR, hot_reset, RAS, DMA... │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Design Principles

1. **C++ Implementation:** All internal components are C++ classes (not SystemC modules) for performance and simplicity.
2. **Function Callbacks:** Internal communication uses C++ function callbacks instead of TLM sockets.
3. **RAII Memory Management:** Smart pointers (`std::unique_ptr`) for automatic resource cleanup.
4. **Address-Based Routing:** Upper address bits [63:60] determine routing paths.
5. **Signal-Based Control:** Control plane uses `sc_signal` for isolation, reset, and configuration tracking.

---

## 3. Block Diagram

### 3.1 Tile-Level Block Diagram

```
                          ┌──────────────────────────────────────────────┐
                          │        KERAUNOS PCIE TILE                    │
                          │                                              │
  ┌──────────┐            │  ┌──────────────────────────────────────┐   │
  │   NOC    │◄───────────┼──┤   NOC-PCIE Switch                    │   │
  │ Network  │────────────┼─►│   - Route decode [63:60]             │   │
  └──────────┘            │  │   - Status reg (0xE/0xF)             │   │
      ▲ ▼                 │  │   - Isolation gating                 │   │
      │ │                 │  └────┬───┬───┬───────────────┬─────────┘   │
      │ │                 │       │   │   │               │             │
      │ │                 │  ┌────▼───▼───▼───┐      ┌────▼──────┐      │
      │ │                 │  │  Inbound TLBs  │      │  Bypass   │      │
      │ │                 │  │  - TLB Sys In0 │      │  Paths    │      │
      │ │                 │  │  - TLB App In0 │      │           │      │
      │ │                 │  │    [0-3] (4x)  │      └─────┬─────┘      │
      │ │                 │  │  - TLB App In1 │            │            │
      │ │                 │  └───────┬────────┘            │            │
      │ │                 │          │                     │            │
      │ │                 │  ┌───────▼─────────────────────▼────────┐   │
      │ └─────────────────┼──┤   NOC-IO Switch                      │   │
      │                   │  │   - NOC-N output                     │   │
      └───────────────────┼──┤   - MSI Relay input                  │   │
                          │  │   - TLB App Out0 route               │   │
                          │  └──────────┬────────────┬──────────────┘   │
                          │             │            │                  │
                          │    ┌────────▼────┐  ┌────▼────────────┐    │
                          │    │  MSI Relay  │  │ Outbound TLBs   │    │
                          │    │  Unit       │  │ - TLB App Out0  │    │
                          │    │  - MSI-X    │  │ - TLB Sys Out0  │    │
                          │    │  - PBA      │  │ - TLB App Out1  │    │
                          │    └─────────────┘  └─────────────────┘    │
                          │                                             │
  ┌──────────┐            │  ┌──────────────────────────────────────┐  │
  │   SMN    │◄───────────┼──┤   SMN-IO Switch                      │  │
  │ Network  │────────────┼─►│   - Config Reg (0x18000000)          │  │
  └──────────┘            │  │   - TLB Config (0x18200000-0x182Fxxxx)│  │
                          │  │   - MSI Config (0x18800000)          │  │
                          │  │   - SII Config (0x18101000)          │  │
                          │  │   - PHY Config (0x18102000/0x18103000)│  │
                          │  │   - TLB Sys bypass                   │  │
                          │  └─────┬────┬────┬────┬──────┬──────────┘  │
                          │        │    │    │    │      │             │
                          │   ┌────▼┐ ┌─▼──┐ │ ┌──▼───┐ │             │
                          │   │Config│ │SII │ │ │ MSI  │ │             │
                          │   │ Reg  │ │Block│ │ │Relay │ │             │
                          │   │Block │ │     │ │ │      │ │             │
                          │   └──────┘ └────┘ │ └──────┘ │             │
                          │                   │          │             │
                          │              ┌────▼──┐  ┌────▼─────┐       │
                          │              │ TLBs  │  │PCIe PHY  │       │
                          │              │Config │  │          │       │
                          │              └───────┘  └──────────┘       │
                          │                                             │
  ┌──────────┐            │  ┌──────────────────────────────────────┐  │
  │   PCIe   │◄───────────┼──┤   PCIe Controller Interface          │  │
  │Controller│────────────┼─►│   - NOC-PCIE Switch (route from PCIe)│  │
  └──────────┘            │  └──────────────────────────────────────┘  │
                          │                                             │
                          │  ┌──────────────────────────────────────┐  │
     Control Inputs       │  │  Clock & Reset Control               │  │
     ────────────────────►│  │  - cold_reset_n                      │  │
     - cold_reset_n       │  │  - warm_reset_n                      │  │
     - warm_reset_n       │  │  - isolate_req (clears enables)      │  │
     - isolate_req        │  └──────────────────────────────────────┘  │
     - pcie_cii_hv        │                                             │
     - pcie_cii_hdr_type  │  ┌──────────────────────────────────────┐  │
     - pcie_cii_hdr_addr  │  │  Interrupt Forwarding                │  │
     - pcie_flr_request   │  │  - config_update (from SII CII)      │  │
     - pcie_hot_reset     │  │  - function_level_reset (FLR fwd)    │  │
     - pcie_ras_error     │  │  - hot_reset_requested (fwd)         │  │
     - pcie_dma_completion│  │  - ras_error (fwd)                   │  │
     - pcie_misc_int      │  │  - dma_completion (fwd)              │  │
                          │  │  - controller_misc_int (fwd)         │  │
                          │  └──────────────────────────────────────┘  │
                          │                                             │
                          └─────────────────────────────────────────────┘
```

### 3.2 Data Flow Paths

**Inbound Path (PCIe → NOC):**
```
PCIe Controller → NOC-PCIE Switch → Inbound TLB → NOC-IO Switch → NOC Network
                      │                   │
                      │                   └─► Address Translation
                      └─► Route decode [63:60], enable gating
```

**Outbound Path (NOC → PCIe):**
```
NOC Network → NOC-IO Switch → Outbound TLB → NOC-PCIE Switch → PCIe Controller
                   │               │
                   │               └─► Address Translation
                   └─► MSI input path (0x18800000)
```

**Configuration Path (SMN → Tile Internals):**
```
SMN Network → SMN-IO Switch ──┬─► Config Reg Block (0x18000000)
                              ├─► TLB Config (0x18200000-0x182Fxxxx)
                              ├─► MSI Relay Config (0x18800000)
                              ├─► SII Config APB (0x18101000)
                              ├─► PCIe PHY APB (0x18102000)
                              ├─► PCIe PHY AHB (0x18103000)
                              └─► TLB Sys bypass (inbound/outbound)
```

**Bypass Paths:**
```
PCIe Controller → NOC-PCIE Switch ──┬─► Bypass App (route 0x8) → NOC-IO Switch → NOC
                                    └─► Bypass Sys (route 0x9) → SMN-IO Switch → SMN
```

---

## 4. Module Descriptions

### 4.1 NOC-PCIE Switch

**Purpose:** Routes traffic from PCIe controller to appropriate destination (TLBs, bypass paths, status register).

**Key Features:**
- **Route Decode:** Address bits [63:60] determine destination
- **Status Register:** Routes 0xE and 0xF provide read-only system status
- **Isolation Gating:** `isolate_req` or `!pcie_inbound_enable` blocks all traffic
- **Enable Control:** `pcie_inbound_app_enable` gates application traffic

**Route Table:**

| Route Bits [63:60] | Destination | Description |
|-------------------|-------------|-------------|
| 0x0 | TLB App In0 [0] | Application inbound TLB instance 0 |
| 0x1 | TLB App In1 | Application inbound TLB (large pages) |
| 0x4 | TLB Sys In0 | System inbound TLB |
| 0x8 | Bypass App | Direct to NOC-IO (no translation) |
| 0x9 | Bypass Sys | Direct to SMN-IO (no translation) |
| 0xE, 0xF | Status Register | Read-only status (if `system_ready`) |
| Other | DECERR | TLM_ADDRESS_ERROR_RESPONSE |

**Status Register Format (32-bit):**
```
[0]    system_ready
[1]    pcie_outbound_app_enable
[2]    pcie_inbound_app_enable
[31:3] Reserved (0)
```

**Files:**
- `keraunos_pcie_noc_pcie_switch.h`
- `keraunos_pcie_noc_pcie_switch.cpp`

---

### 4.2 NOC-IO Switch

**Purpose:** Routes traffic between NOC network and internal tile resources (MSI relay, outbound TLBs).

**Key Features:**
- **Address-Based Routing:** Upper address bits determine destination
- **MSI Input Path:** Routes MSI trigger writes to MSI Relay at 0x18800000
- **Outbound TLB Path:** Routes traffic to application outbound TLB for PCIe egress

**Route Logic:**
```
if (addr & 0x18800000) → MSI Relay (process_msi_input)
else                   → TLB App Out0 (process_outbound_traffic)
```

**Files:**
- `keraunos_pcie_noc_io_switch.h`
- `keraunos_pcie_noc_io_switch.cpp`

---

### 4.3 SMN-IO Switch

**Purpose:** Routes SMN configuration traffic to tile configuration registers, TLB config, MSI config, SII, and PCIe PHY.

**Key Features:**
- **Configuration Access:** All tile-level configuration registers
- **TLB Configuration:** 8 TLB instances (5 inbound + 3 outbound)
- **Bypass Data Path:** TLB Sys inbound/outbound data traffic (not config)
- **Address Passthrough Limitation:** Passes full address to callbacks instead of relative offset

**Address Map:**

| Base Address | Size | Destination | Access Type |
|--------------|------|-------------|-------------|
| 0x18000000 | 4KB | Config Reg Block | RW config registers |
| 0x18101000 | 64KB | SII Config (APB) | APB config space |
| 0x18102000 | 4KB | PCIe PHY APB | PHY APB registers |
| 0x18103000 | 4KB | PCIe PHY AHB | PHY AHB registers |
| 0x18200000 | 4KB | TLB Sys In0 Config | TLB config registers |
| 0x18210000 | 4KB | TLB App In0 [0] Config | TLB config (instance 0) |
| 0x18220000 | 4KB | TLB App In0 [1] Config | TLB config (instance 1) |
| 0x18230000 | 4KB | TLB App In0 [2] Config | TLB config (instance 2) |
| 0x18240000 | 4KB | TLB App In0 [3] Config | TLB config (instance 3) |
| 0x18250000 | 4KB | TLB App In1 Config | TLB config |
| 0x18260000 | 4KB | TLB Sys Out0 Config | TLB config |
| 0x18270000 | 4KB | TLB App Out0 Config | TLB config |
| 0x18280000 | 4KB | TLB App Out1 Config | TLB config |
| 0x18800000 | 4KB | MSI Relay Config | MSI-X table, mask, PBA |

**Files:**
- `keraunos_pcie_smn_io_switch.h`
- `keraunos_pcie_smn_io_switch.cpp`

---

### 4.4 Translation Lookaside Buffers (TLBs)

**Purpose:** Translate between PCIe IATU (Internal Address Translation Unit) addresses and system physical addresses.

**TLB Instances (8 total):**

| Name | Direction | Count | Page Shift | Index Bits | Entries |
|------|-----------|-------|------------|------------|---------|
| TLB Sys In0 | Inbound | 1 | 14 | [19:14] | 64 |
| TLB App In0 | Inbound | 4 | 24 | [29:24] | 64 each |
| TLB App In1 | Inbound | 1 | 33 | [38:33] | 64 |
| TLB Sys Out0 | Outbound | 1 | 14 | [19:14] | 64 |
| TLB App Out0 | Outbound | 1 | 24 | [29:24] | 64 |
| TLB App Out1 | Outbound | 1 | 33 | [38:33] | 64 |

**Translation Algorithm (Inbound):**
```
1. index = (iatu_address >> page_shift) & 0x3F
2. if (index >= 64 || !entries[index].valid) → DECERR
3. translated_addr = (entries[index].addr << 12) | (iatu_address & page_mask)
4. axuser = entries[index].attr
5. Continue to next stage with translated address
```

**TLB Entry Structure:**
```
valid:       1 bit    - Entry valid flag
addr:        52 bits  - Translated address [63:12]
attr:        32 bits  - AxUSER attributes
```

**Default Configuration:**
- Entry 0: valid=true (default-initialized, allows basic traffic flow)
- Entries 1-63: valid=false (must be configured via SMN)

**Files:**
- `keraunos_pcie_inbound_tlb.h` / `.cpp` (5 inbound TLB classes)
- `keraunos_pcie_outbound_tlb.h` / `.cpp` (3 outbound TLB classes)

---

### 4.5 MSI Relay Unit

**Purpose:** Relay MSI-X interrupts from software (via NOC) to PCIe controller.

**Key Features:**
- **MSI-X Table:** 16 entries (address, data, per-vector mask)
- **Pending Bit Array (PBA):** 16-bit PBA for pending MSI tracking
- **Global Mask:** MSI-X mask and enable control
- **Outstanding Counter:** Tracks in-flight MSIs

**MSI Generation Flow:**
```
1. Software writes to MSI input address (0x18800000) via NOC
2. MSI relay checks: msix_enable && !global_mask && !vector_mask
3. If conditions met: generate MSI to PCIe controller
4. Increment msi_outstanding counter
5. Set PBA bit if unable to send immediately
```

**Configuration Registers (via SMN at 0x18800000):**
- **MSI-X Table:** Offset 0x0000-0x00FF (16 entries × 16 bytes)
  - Entry format: [63:0] address, [95:64] data, [96] mask, [127:97] reserved
- **Pending Bit Array:** Offset 0x0100-0x0103 (16 bits, read-only)
- **Outstanding Counter:** Internal counter (read via status query)

**Known Limitation (Address Passthrough):**
- MSI input path expects offset=0 for PBA bit setting
- NOC-IO switch passes full address 0x18800000 to `process_msi_input()`
- PBA bits are never set due to offset mismatch
- MSI generation still functional for basic relay

**Files:**
- `keraunos_pcie_msi_relay.h`
- `keraunos_pcie_msi_relay.cpp`

---

### 4.6 System Information Interface (SII) Block

**Purpose:** Track PCIe configuration space updates via CII (Configuration Intercept Interface) and generate `config_update` interrupt.

**Key Features:**
- **CII Tracking:** Monitors PCIe controller config writes (type 0x04, first 128 bytes)
- **Accumulated Bitmask:** 32-bit `cfg_modified` register with RW1C semantics
- **Interrupt Generation:** Asserts `config_update` when any `cfg_modified` bit is set
- **APB Configuration:** Bus/device number, device type, system interrupt control

**CII Detection Logic:**
```
1. Wait for pcie_cii_hv = true (CII header valid)
2. Check pcie_cii_hdr_type == 0x04 (config write)
3. Check pcie_cii_hdr_addr < 0x080 (first 128 bytes of config space)
4. If all true: reg_index = addr[6:2], set cfg_modified[reg_index] = 1
5. Assert config_update = true
```

**CDC Synchronization:**
- CII inputs from PCIe clock domain
- Synchronize `cfg_modified` bitmask to APB clock domain
- Write synchronized value to SCML2 memory for software readback

**APB Registers (via SMN at 0x18101000):**

| Offset | Register | Access | Description |
|--------|----------|--------|-------------|
| 0x0000 | CORE_CONTROL | RW | Device type [2:0]: 0x4=RP, others=EP |
| 0x0004 | CFG_MODIFIED | RW1C | Config modified bitmask (32 bits) |
| 0x0008 | BUS_DEV_NUM | RW | Bus number [15:8], Device number [7:0] |

**Interrupt Clear:**
- Software write-1-to-clear (RW1C) to CFG_MODIFIED register
- **Limitation:** Address passthrough blocks SMN writes from reaching SII
- Alternative: `pcie_controller_reset_n` pulse clears `cfg_modified` and deasserts `config_update`

**Files:**
- `keraunos_pcie_sii.h`
- `keraunos_pcie_sii.cpp`

---

### 4.7 Configuration Register Block

**Purpose:** Tile-level control registers for system readiness and isolation.

**Key Registers (accessed via SMN at 0x18000000):**

| Offset | Register | Bits | Description |
|--------|----------|------|-------------|
| 0x0000 | SYSTEM_CONTROL | [0] | `system_ready` - Master enable |
| 0x0004 | ENABLE_CONTROL | [1:0] | `pcie_outbound_app_enable`, `pcie_inbound_app_enable` |
| 0x0008 | ISOLATION_CONTROL | [0] | `isolate_req` - Isolation request |

**Isolation Mechanism:**
```
When isolate_req is asserted (set to true):
1. system_ready = false
2. pcie_outbound_app_enable = false
3. pcie_inbound_app_enable = false
4. NOC-PCIE switch blocks all inbound traffic (returns DECERR)

When isolate_req is deasserted (set to false):
1. isolate_req flag cleared
2. Enables NOT restored (permanent state loss)
3. Requires external software reconfiguration to restore traffic flow
```

**Known Limitation:**
- No reset handler to restore enables after isolation
- De-asserting `isolate_req` does not restore `system_ready` or enable bits
- Cold/warm reset does not restore enables (only initialized in constructor)

**Files:**
- `keraunos_pcie_config_reg.h`
- `keraunos_pcie_config_reg.cpp`

---

### 4.8 Clock and Reset Control

**Purpose:** Manage clock distribution and reset sequencing for tile internals.

**Clocks:**
- **pcie_core_clk:** PCIe controller core clock input
- **axi_clk:** AXI interface clock input
- **pcie_clock_:** Internal PCIe clock signal (derived)
- **ref_clock_:** Reference clock for PLL

**Resets:**
- **cold_reset_n:** Full tile reset (active low)
  - Resets: `pcie_sii_reset_ctrl_`, `pcie_reset_ctrl_`
  - Does NOT reset: enables, TLBs, `system_ready`
- **warm_reset_n:** Warm reset (active low)
  - Resets: Same as cold reset
- **pcie_controller_reset_n:** PCIe controller-specific reset
  - Clears SII `cfg_modified` bitmask and deasserts `config_update`

**Reset Scope:**
```
Cold/Warm Reset:
- Affects: Clock/Reset control block internal state
- Preserves: Configuration registers, TLB entries, enable flags

PCIe Controller Reset:
- Affects: SII cfg_modified register, config_update interrupt
- Preserves: All other state
```

**Files:**
- `keraunos_pcie_clock_reset.h`
- `keraunos_pcie_clock_reset.cpp`

---

### 4.9 PCIe PHY Model

**Purpose:** Placeholder model for PCIe PHY (SERDES) configuration.

**Interfaces:**
- **APB Configuration:** Accessed via SMN at 0x18102000
- **AHB Configuration:** Accessed via SMN at 0x18103000

**Functionality:**
- Provides configuration register space for PHY settings
- No actual signal-level simulation (TLM abstraction)
- Returns TLM_OK_RESPONSE for all accesses

**Files:**
- `keraunos_pcie_phy.h`
- `keraunos_pcie_phy.cpp`

---

### 4.10 PLL/CGM (Clock Generation Module)

**Purpose:** Placeholder model for PLL and clock generation.

**Functionality:**
- Manages internal clock generation from reference clock
- Provides clock gating control
- No detailed implementation (placeholder for future expansion)

**Files:**
- `keraunos_pcie_pll_cgm.h`
- `keraunos_pcie_pll_cgm.cpp`

---

## 5. Interface Specifications

### 5.1 External TLM Sockets

| Socket Name | Type | Width | Direction | Description |
|-------------|------|-------|-----------|-------------|
| `noc_n_target` | Target | 64-bit | Inbound | NOC → Tile (outbound traffic) |
| `noc_n_initiator` | Target* | 64-bit | Outbound | Tile → NOC (inbound traffic) |
| `smn_n_target` | Target | 64-bit | Inbound | SMN → Tile (config/data) |
| `smn_n_initiator` | Target* | 64-bit | Outbound | Tile → SMN (bypass/data) |
| `pcie_controller_target` | Target | 64-bit | Inbound | PCIe → Tile (inbound traffic) |
| `pcie_controller_initiator` | Target* | 64-bit | Outbound | Tile → PCIe (outbound traffic) |

*Note: Named "initiator" but implemented as target sockets for test harness convenience.

**TLM Protocol:**
- **Transport:** Blocking transport (`b_transport`)
- **Address:** 64-bit byte addressing
- **Data:** Variable length (1, 2, 4, 8 bytes typical)
- **Response:** TLM_OK_RESPONSE or TLM_ADDRESS_ERROR_RESPONSE

---

### 5.2 Control Input Signals

| Signal Name | Type | Width | Description |
|-------------|------|-------|-------------|
| `cold_reset_n` | sc_in | 1 | Cold reset (active low) |
| `warm_reset_n` | sc_in | 1 | Warm reset (active low) |
| `isolate_req` | sc_in | 1 | Isolation request (clears enables) |
| `pcie_cii_hv` | sc_in | 1 | CII header valid |
| `pcie_cii_hdr_type` | sc_in | 5 | CII header type (0x04=config write) |
| `pcie_cii_hdr_addr` | sc_in | 12 | CII header address [11:0] |
| `pcie_core_clk` | sc_in | 1 | PCIe core clock |
| `pcie_controller_reset_n` | sc_in | 1 | PCIe controller reset (active low) |
| `pcie_flr_request` | sc_in | 1 | Function-level reset request |
| `pcie_hot_reset` | sc_in | 1 | Hot reset indication |
| `pcie_ras_error` | sc_in | 1 | RAS error indication |
| `pcie_dma_completion` | sc_in | 1 | DMA completion indication |
| `pcie_misc_int` | sc_in | 1 | Miscellaneous interrupt |
| `axi_clk` | sc_in | 1 | AXI clock |

**Signal Propagation:**
- All signals require `sc_core::wait(SC_ZERO_TIME)` to propagate changes
- `signal_update_process()` sensitive to all control inputs
- Updates internal state and output signals each delta cycle

---

### 5.3 Output Signals

| Signal Name | Type | Width | Description |
|-------------|------|-------|-------------|
| `pcie_app_bus_num` | sc_out | 8 | Application bus number (from SII) |
| `pcie_app_dev_num` | sc_out | 8 | Application device number (from SII) |
| `pcie_device_type` | sc_out | 1 | Device type: 0=EP, 1=RP (from SII) |
| `pcie_sys_int` | sc_out | 1 | System interrupt (from SII, always 0) |
| `function_level_reset` | sc_out | 1 | FLR forwarded from PCIe controller |
| `hot_reset_requested` | sc_out | 1 | Hot reset forwarded from PCIe controller |
| `config_update` | sc_out | 1 | Config update interrupt (from SII CII) |
| `ras_error` | sc_out | 1 | RAS error forwarded from PCIe controller |
| `dma_completion` | sc_out | 1 | DMA completion forwarded from PCIe controller |
| `controller_misc_int` | sc_out | 1 | Misc interrupt forwarded from PCIe controller |
| `noc_timeout` | sc_out | 3 | NOC timeout status (reserved) |

**Output Update:**
- All outputs driven by `signal_update_process()`
- Updated on every delta cycle when inputs change
- Initialized to default values in `end_of_elaboration()`

---

### 5.4 Internal Signals

| Signal Name | Type | Width | Description |
|-------------|------|-------|-------------|
| `system_ready_` | sc_signal | 1 | Master enable for tile (affects NOC-PCIE routing) |
| `pcie_outbound_app_enable_` | sc_signal | 1 | Enable outbound application traffic |
| `pcie_inbound_app_enable_` | sc_signal | 1 | Enable inbound application traffic |
| `msix_enable_` | sc_signal | 1 | MSI-X global enable (not externally accessible) |
| `msix_mask_` | sc_signal | 1 | MSI-X global mask (not externally accessible) |
| `setip_` | sc_signal | 16 | Set interrupt pending (reserved) |
| `pcie_clock_` | sc_signal | 1 | Internal PCIe clock |
| `ref_clock_` | sc_signal | 1 | Reference clock |
| `pcie_sii_reset_ctrl_` | sc_signal | 1 | SII reset control |
| `pcie_reset_ctrl_` | sc_signal | 1 | PCIe reset control |

---

## 6. Address Map

### 6.1 Inbound Address Map (PCIe → NOC/SMN)

**Route Decode (bits [63:60]):**

| Route | Address Range | Destination | Notes |
|-------|--------------|-------------|-------|
| 0x0 | 0x0xxxxxxxxxxxxxxx | TLB App In0 [0] | 16MB pages, index [29:24] |
| 0x1 | 0x1xxxxxxxxxxxxxxx | TLB App In1 | 8GB pages, index [38:33] |
| 0x4 | 0x4xxxxxxxxxxxxxxx | TLB Sys In0 | 16KB pages, index [19:14] |
| 0x8 | 0x8xxxxxxxxxxxxxxx | Bypass App (NOC-IO) | No translation |
| 0x9 | 0x9xxxxxxxxxxxxxxx | Bypass Sys (SMN-IO) | No translation |
| 0xE | 0xExxxxxxxxxxxxxxx | Status Register (RO) | Read-only, 32-bit status |
| 0xF | 0xFxxxxxxxxxxxxxxx | Status Register (RO) | Same as 0xE |
| Other | - | DECERR | TLM_ADDRESS_ERROR_RESPONSE |

### 6.2 Configuration Address Map (SMN)

**SMN-IO Switch Routes (base = 0x18000000):**

| Base Address | Size | Module | Registers |
|--------------|------|--------|-----------|
| 0x18000000 | 4KB | Config Reg Block | System control, enables, isolation |
| 0x18101000 | 64KB | SII APB Config | CORE_CONTROL, CFG_MODIFIED, BUS_DEV_NUM |
| 0x18102000 | 4KB | PCIe PHY APB | PHY APB registers |
| 0x18103000 | 4KB | PCIe PHY AHB | PHY AHB registers |
| 0x18200000 | 4KB | TLB Sys In0 Config | TLB entry config (64 entries × 16 bytes) |
| 0x18210000 | 4KB | TLB App In0 [0] Config | TLB entry config |
| 0x18220000 | 4KB | TLB App In0 [1] Config | TLB entry config |
| 0x18230000 | 4KB | TLB App In0 [2] Config | TLB entry config |
| 0x18240000 | 4KB | TLB App In0 [3] Config | TLB entry config |
| 0x18250000 | 4KB | TLB App In1 Config | TLB entry config |
| 0x18260000 | 4KB | TLB Sys Out0 Config | TLB entry config |
| 0x18270000 | 4KB | TLB App Out0 Config | TLB entry config |
| 0x18280000 | 4KB | TLB App Out1 Config | TLB entry config |
| 0x18800000 | 4KB | MSI Relay Config | MSI-X table, PBA, mask control |

**Address Passthrough Issue:**
- SMN-IO switch passes **full address** to config callbacks
- Callbacks expect **offset** within 4KB range
- Example: Write to 0x18800000 passes offset=0x18800000 (not 0x0000)
- Impact: Most config writes are rejected or routed incorrectly

### 6.3 MSI Relay Address Map (0x18800000)

| Offset Range | Size | Description |
|--------------|------|-------------|
| 0x0000-0x000F | 16B | MSI-X Table Entry 0 |
| 0x0010-0x001F | 16B | MSI-X Table Entry 1 |
| ... | ... | ... |
| 0x00F0-0x00FF | 16B | MSI-X Table Entry 15 |
| 0x0100-0x0103 | 4B | Pending Bit Array (PBA) - read-only |
| 0x0104-0x0FFF | - | Reserved |

**MSI-X Table Entry Format (16 bytes):**
```
[63:0]   Message Address (64-bit)
[95:64]  Message Data (32-bit)
[96]     Per-Vector Mask (1=masked)
[127:97] Reserved
```

---

## 7. Data Flow

### 7.1 Inbound Data Flow (PCIe → NOC)

**Scenario:** PCIe controller initiates memory write to NOC memory.

```
Step 1: PCIe Controller → pcie_controller_target socket
        trans.address = 0x1000000000001000  // Route=0x1 (TLB App In1)
        trans.command = TLM_WRITE_COMMAND
        trans.data    = {payload}

Step 2: KeraunosPcieTile::pcie_controller_target_b_transport()
        → noc_pcie_switch_->route_from_pcie(trans, delay)

Step 3: NocPcieSwitch::route_from_pcie()
        - Check: isolate_req || !pcie_inbound_enable → BLOCKED if true
        - Extract route_bits = (addr >> 60) & 0xF = 0x1
        - Route decision: case 0x1 → TLB_APP_1

Step 4: NocPcieSwitch calls tlb_app_inbound1_ callback
        → TLBAppIn1::process_inbound_traffic(trans, delay)

Step 5: TLBAppIn1::process_inbound_traffic()
        - index = (0x1000000000001000 >> 33) & 0x3F = 0
        - Check entries[0].valid → TRUE (default)
        - translated_addr = (entries[0].addr << 12) | (0x1000000000001000 & page_mask)
        - trans.set_address(translated_addr)
        - trans.set_response_status(TLM_OK_RESPONSE)
        - Return to switch

Step 6: NocPcieSwitch forwards to noc_io_ callback
        → NocIoSwitch::route_from_inbound_tlb(trans, delay)

Step 7: NocIoSwitch::route_from_inbound_tlb()
        - Routes to NOC-N output
        - In test environment: returns TLM_OK_RESPONSE

Step 8: Response propagates back to PCIe controller
        trans.get_response_status() = TLM_OK_RESPONSE
```

### 7.2 Outbound Data Flow (NOC → PCIe)

**Scenario:** NOC initiates memory read from PCIe device memory.

```
Step 1: NOC → noc_n_target socket
        trans.address = 0x10001000000000  // Outbound traffic
        trans.command = TLM_READ_COMMAND

Step 2: KeraunosPcieTile::noc_n_target_b_transport()
        → noc_io_switch_->route_from_noc(trans, delay)

Step 3: NocIoSwitch::route_from_noc()
        - Check address range
        - Route to TLB App Out0 callback

Step 4: TLBAppOut0::process_outbound_traffic(trans, delay)
        - index = (addr >> 24) & 0x3F
        - Lookup entry, translate address
        - Set translated address in trans

Step 5: Route to NOC-PCIE switch
        → noc_pcie_switch_->route_to_pcie(trans, delay)

Step 6: NocPcieSwitch::route_to_pcie()
        - Forwards to pcie_controller_ callback
        - In real system: sends to PCIe controller
        - In test: returns TLM_OK_RESPONSE

Step 7: Response propagates back to NOC
```

### 7.3 Configuration Access Flow (SMN → Config Registers)

**Scenario:** Software writes to SII CFG_MODIFIED register via SMN.

```
Step 1: SMN → smn_n_target socket
        trans.address = 0x18101004  // SII CFG_MODIFIED register
        trans.command = TLM_WRITE_COMMAND
        trans.data    = {0x00000010}  // RW1C clear bit 4

Step 2: KeraunosPcieTile::smn_n_target_b_transport()
        → smn_io_switch_->route_from_smn(trans, delay)

Step 3: SmnIoSwitch::route_from_smn()
        - Check addr == 0x18101004
        - Route to SII config callback
        - Call: sii_block_->process_apb_access(trans, delay)

Step 4: SiiBlock::process_apb_access()
        - ISSUE: Receives addr = 0x18101004 (full address)
        - Expects offset = 0x0004 (relative to 0x18101000 base)
        - Offset check fails: 0x18101004 > 64KB SCML2 memory range
        - Returns: TLM_ADDRESS_ERROR_RESPONSE

Step 5: Response: Write rejected due to address passthrough

Result: Config write blocked by address passthrough limitation.
```

### 7.4 MSI Generation Flow (NOC → PCIe via MSI Relay)

**Scenario:** Software triggers MSI by writing to MSI input address.

```
Step 1: NOC → noc_n_target socket
        trans.address = 0x18800000  // MSI input address
        trans.command = TLM_WRITE_COMMAND
        trans.data    = {0x00000003}  // Vector 3

Step 2: NocIoSwitch::route_from_noc()
        - Detects addr & 0x18800000
        - Routes to MSI relay input callback
        - Call: msi_relay_->process_msi_input(trans, delay)

Step 3: MsiRelayUnit::process_msi_input()
        - ISSUE: Receives offset = 0x18800000 (expects 0x0000)
        - Offset check: if (offset != 0) → skip PBA bit set
        - PBA bit NOT set due to mismatch
        - Check: msix_enable && !global_mask && !vector_mask
        - If all true: generate MSI transaction

Step 4: MSI Relay generates TLM transaction
        - trans.address = msix_table[vector].address
        - trans.data    = msix_table[vector].data
        - Forwards to noc_pcie_switch_->route_to_pcie()

Step 5: NOC-PCIE switch forwards to PCIe controller
        - PCIe controller processes MSI

Result: MSI successfully delivered, but PBA not updated due to address issue.
```

### 7.5 CII Interrupt Flow (PCIe Config Write → config_update)

**Scenario:** PCIe host writes to device config space, SII detects and asserts interrupt.

```
Step 1: PCIe Controller signals CII inputs
        pcie_cii_hv.write(true)
        pcie_cii_hdr_type.write(0x04)     // Config write
        pcie_cii_hdr_addr.write(0x010)    // Config offset 0x10

Step 2: signal_update_process() triggered (sensitive to CII inputs)
        - Reads CII input signals
        - Calls: sii_block_->set_cii_hv(true)
        - Calls: sii_block_->set_cii_hdr_type(0x04)
        - Calls: sii_block_->set_cii_hdr_addr(0x010)

Step 3: signal_update_process() calls sii_block_->update()

Step 4: SiiBlock::update() - CII Tracking Phase
        - Check: cii_hv == true → YES
        - Check: cii_hdr_type == 0x04 → YES
        - Check: cii_hdr_addr < 0x080 → YES (0x010 < 0x080)
        - reg_index = 0x010 >> 2 = 4
        - cii_new_bits = (1 << 4) = 0x00000010

Step 5: SiiBlock::update() - cfg_modified Update Phase
        - cfg_modified = (cfg_modified & ~cii_clear) | cii_new_bits
        - cfg_modified |= 0x00000010 (bit 4 now set)

Step 6: SiiBlock::update() - Interrupt Generation Phase
        - config_int_ = (cfg_modified != 0) → TRUE

Step 7: signal_update_process() reads SII outputs
        - bool cfg_upd = sii_block_->get_config_int() → TRUE
        - config_update.write(true)

Step 8: config_update signal propagates to system
        - External interrupt controller sees config_update assertion
        - Software can read CFG_MODIFIED via SII APB (if not blocked by passthrough)

Step 9: Interrupt Clear (via PCIe controller reset)
        pcie_controller_reset_n.write(false)  // Assert reset
        wait(SC_ZERO_TIME)
        pcie_controller_reset_n.write(true)   // Deassert reset
        - SiiBlock::update() sees !reset_n_ → clears cfg_modified
        - config_update deasserts

Result: config_update interrupt lifecycle complete.
```

---

## 8. Clock and Reset Strategy

### 8.1 Clock Domains

The tile operates across multiple clock domains:

| Clock Domain | Source Signal | Modules Affected |
|--------------|---------------|------------------|
| PCIe Core | `pcie_core_clk` | SII (CII tracking), PCIe PHY, NOC-PCIE switch |
| AXI | `axi_clk` | NOC-IO switch, SMN-IO switch, TLBs |
| Internal | `pcie_clock_` | PLL/CGM, clock distribution |
| Reference | `ref_clock_` | PLL input |

**Clock Domain Crossing (CDC):**
- **SII CII → APB:** `cfg_modified` bitmask synchronized from PCIe clock to APB clock domain
- **Method:** Double-flop synchronizer (implicit in `SiiBlock::update()` CDC phase)

### 8.2 Reset Strategy

**Reset Hierarchy:**

```
cold_reset_n (top-level)
    ├─► ClockResetControl::pcie_sii_reset_ctrl_
    └─► ClockResetControl::pcie_reset_ctrl_

pcie_controller_reset_n (PCIe-specific)
    └─► SiiBlock::reset_n_ (clears cfg_modified)

isolate_req (isolation, not true reset)
    ├─► ConfigRegBlock::system_ready = false
    ├─► ConfigRegBlock::pcie_inbound_app_enable = false
    └─► ConfigRegBlock::pcie_outbound_app_enable = false
```

**Reset Effects:**

| Reset Signal | Affects | Preserves |
|--------------|---------|-----------|
| `cold_reset_n` | Clock/Reset control registers | Config registers, TLBs, enables, `system_ready` |
| `warm_reset_n` | Same as cold reset | Same as cold reset |
| `pcie_controller_reset_n` | SII `cfg_modified`, `config_update` | All other state |
| `isolate_req` | Clears all enable flags | TLB config, register values |

**Reset Sequence (Cold Reset):**

```
1. Assert cold_reset_n = false
2. wait(SC_ZERO_TIME)  // Allow reset to propagate
3. All clock/reset control state cleared
4. Deassert cold_reset_n = true
5. wait(SC_ZERO_TIME)  // Allow deassert to propagate
6. System operational (with preserved config)
```

**Isolation vs. Reset:**
- **Isolation:** Disables traffic flow, preserves config (but cannot be undone)
- **Reset:** Clears control state, preserves config and enables
- **Neither:** Clears TLB entries or configuration registers

### 8.3 Power-On Initialization

**Constructor Initialization:**
```
system_ready_ = true
pcie_outbound_app_enable_ = true
pcie_inbound_app_enable_ = true
TLB entry[0].valid = true (all TLBs)
TLB entries[1-63].valid = false
```

**end_of_elaboration():**
```
All output signals initialized to 0/false
pcie_app_bus_num = 0
pcie_app_dev_num = 0
pcie_device_type = false (EP mode)
config_update = false
```

---

## 9. Configuration and Control

### 9.1 Tile Initialization Sequence

**Recommended Startup Flow:**

```
Step 1: Assert cold_reset_n
        cold_reset_n.write(false)
        wait(SC_ZERO_TIME)

Step 2: Configure TLB entries (via SMN)
        For each TLB instance:
            Write to SMN address 0x18200000-0x18280000
            - Entry valid bit
            - Translated address [63:12]
            - AxUSER attributes

Step 3: Configure MSI Relay (via SMN)
        Write to SMN address 0x18800000-0x188000FF
            - MSI-X table entries (address, data, mask)

Step 4: Configure SII (via SMN)
        Write to SMN address 0x18101000
            - CORE_CONTROL (device type)
            - BUS_DEV_NUM (bus/device numbers)

Step 5: Deassert cold_reset_n
        cold_reset_n.write(true)
        wait(SC_ZERO_TIME)

Step 6: Verify system_ready
        Read status register via PCIe at 0xE000000000000000
        Check bit[0] == 1

Step 7: Enable traffic flow
        Write to Config Reg Block at 0x18000000
            - Set system_ready = true
            - Set pcie_inbound_app_enable = true
            - Set pcie_outbound_app_enable = true

Step 8: System ready for traffic
```

**Note:** Due to address passthrough limitation, Steps 2-4 may not work as intended. Default configuration (entry 0 valid) allows basic traffic flow.

### 9.2 Runtime Control

**Enable/Disable Traffic:**
```
Write to 0x18000000 (Config Reg Block):
- Offset 0x0000: system_ready [0]
- Offset 0x0004: pcie_outbound_app_enable [1], pcie_inbound_app_enable [0]
```

**Isolation Mode:**
```
Write to 0x18000008 (Config Reg Block):
- Offset 0x0008: isolate_req [0] = 1

Effect:
- All enables cleared
- Traffic blocked at NOC-PCIE switch
- CANNOT BE UNDONE (no recovery mechanism)
```

**TLB Reconfiguration:**
```
Write to TLB config address (0x18200000-0x18280000):
- Entry offset = entry_index * 16
- [0:0]     valid bit
- [63:12]   translated address
- [95:64]   AxUSER attributes
```

**MSI-X Configuration:**
```
Write to MSI-X table (0x18800000-0x188000FF):
- Entry offset = vector * 16
- [63:0]    Message Address
- [95:64]   Message Data
- [96]      Per-Vector Mask (0=enabled, 1=masked)
```

**CII Interrupt Clear:**
```
Method 1 (Blocked by passthrough):
    Write-1-to-clear to 0x18101004 (SII CFG_MODIFIED)
    - Each bit set to 1 clears corresponding bit in cfg_modified

Method 2 (Functional):
    Pulse pcie_controller_reset_n:
        pcie_controller_reset_n.write(false)
        wait(SC_ZERO_TIME)
        pcie_controller_reset_n.write(true)
        wait(SC_ZERO_TIME)
    - Clears all cfg_modified bits
    - Deasserts config_update
```

### 9.3 Status Monitoring

**System Status Register (Route 0xE/0xF, Read-Only):**
```
Read from PCIe: 0xE000000000000000 or 0xF000000000000000

Response (32-bit):
    [0]    system_ready
    [1]    pcie_outbound_app_enable
    [2]    pcie_inbound_app_enable
    [31:3] Reserved (0)
```

**MSI Relay Status:**
```
Read PBA (Pending Bit Array):
    SMN read from 0x18800100-0x18800103
    - 16 bits, one per MSI-X vector
    - Bit set = MSI pending for that vector
    - Note: May always read 0 due to address passthrough

Read Outstanding Counter:
    Internal counter, not externally accessible
```

**SII Status:**
```
Read CFG_MODIFIED:
    SMN read from 0x18101004
    - 32 bits, one per 4-byte register in first 128B of config space
    - Bit set = config register modified since last clear

Read Device Type:
    SMN read from 0x18101000 bits [2:0]
    - 0x4 = Root Port
    - Other = Endpoint
```

---

## 10. References

### 10.1 Related Documents

- **Keraunos PCIe Tile Test Plan:** `/localdev/pdroy/keraunos_pcie_workspace/doc/Keraunos_PCIE_Tile_Testplan.md`
- **Test Implementation:** `/localdev/pdroy/keraunos_pcie_workspace/Keraunos_PCIe_tile/Tests/Unittests/Keranous_pcie_tileTest.cc`
- **Test Guide:** `/localdev/pdroy/keraunos_pcie_workspace/KERAUNOS_PCIE_TEST_GUIDE.md`

### 10.2 Source Code Organization

```
Keraunos_PCIe_tile/
├── SystemC/
│   ├── include/
│   │   ├── keraunos_pcie_tile.h             (Top-level tile)
│   │   ├── keraunos_pcie_noc_pcie_switch.h  (NOC-PCIE switch)
│   │   ├── keraunos_pcie_noc_io_switch.h    (NOC-IO switch)
│   │   ├── keraunos_pcie_smn_io_switch.h    (SMN-IO switch)
│   │   ├── keraunos_pcie_inbound_tlb.h      (5 inbound TLB classes)
│   │   ├── keraunos_pcie_outbound_tlb.h     (3 outbound TLB classes)
│   │   ├── keraunos_pcie_msi_relay.h        (MSI relay unit)
│   │   ├── keraunos_pcie_sii.h              (SII block)
│   │   ├── keraunos_pcie_config_reg.h       (Config register block)
│   │   ├── keraunos_pcie_clock_reset.h      (Clock/reset control)
│   │   ├── keraunos_pcie_pll_cgm.h          (PLL/CGM)
│   │   └── keraunos_pcie_phy.h              (PCIe PHY model)
│   └── src/
│       └── (corresponding .cpp files)
├── Tests/
│   └── Unittests/
│       ├── Keranous_pcie_tileTest.cc        (76 test cases)
│       └── Keranous_pcie_tileTestHarness.h  (Test harness)
└── doc/
    ├── Keraunos_PCIE_Tile_Testplan.md       (Test plan)
    └── Keraunos_PCIe_Tile_HLD.md            (This document)
```

### 10.3 Key Design Decisions

1. **C++ Classes vs. SystemC Modules:**
   - Internal components are C++ classes, not `sc_module`
   - Rationale: Simplified wiring, better performance, no FastBuild instrumentation of internals

2. **Function Callbacks vs. TLM Sockets:**
   - Internal communication uses `std::function` callbacks
   - Rationale: No socket binding overhead, direct function calls, easier debugging

3. **Address Passthrough Instead of Base Stripping:**
   - Switches pass full address to config callbacks
   - Consequence: Most config writes fail offset checks
   - Mitigation: Default initialization allows basic operation without config

4. **Isolation without Recovery:**
   - `isolate_req` permanently clears enables
   - Consequence: De-asserting isolation does not restore traffic
   - Workaround: Avoid isolation in test environment, or accept single-use isolation

5. **CII Tracking as Synchronous Update:**
   - `SiiBlock::update()` combines 3 separate processes into one method
   - Called by `signal_update_process()` each delta cycle
   - Rationale: Simplified CDC, deterministic behavior in TLM environment

6. **TLB Entry 0 Default Valid:**
   - All TLBs initialize entry 0 as valid
   - Rationale: Allows basic traffic flow without configuration
   - Consequence: Address translation not accurate without config, but system stable

### 10.4 Known Limitations and Findings

**Documented in Test Plan (Section 1.5 - Known Findings):**

1. **Address Passthrough:** SMN-IO/NOC-IO switches pass full addresses to callbacks instead of relative offsets → config writes largely non-functional
2. **TLB Configuration Blocked:** TLB config writes via SMN routed to data paths instead of config callbacks
3. **MSI Relay Configuration Limited:** MSI-X enable/mask are internal signals, not exposed; config writes blocked by passthrough
4. **Signal Propagation (RESOLVED):** `sc_core::wait(SC_ZERO_TIME)` required for signal changes to propagate
5. **Isolation Recovery Impossible:** `isolate_req=false` does not restore enables
6. **CII Processing (RESOLVED):** Restored via `SiiBlock::update()` method
7. **MSI PBA Not Verifiable:** PBA bit set requires offset=0, but receives 0x18800000

**Test Coverage:**
- **76 test cases** (41 E2E, 35 directed)
- **251 assertions** (0 failures)
- **100% functional coverage** of testable paths
- **Partial coverage** of blocked features (documented as limitations)

---

## Appendix A: Signal Timing Diagrams

### A.1 CII Interrupt Timing

```
pcie_cii_hv         _____/‾‾‾‾‾‾‾\_____
pcie_cii_hdr_type   _____(0x04)_______
pcie_cii_hdr_addr   _____(0x010)______

wait(SC_ZERO_TIME)  ────────►
                    (inputs propagate)

SiiBlock::update()  ────────────────►
                    (CII tracking, cfg_modified[4]=1)

wait(SC_ZERO_TIME)  ────────────────────►
                    (outputs propagate)

config_update       ___________/‾‾‾‾‾‾‾‾‾‾‾‾‾ (asserted)
```

### A.2 Isolation Sequence

```
isolate_req         _____/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾\_____
wait(SC_ZERO_TIME)  ───────►

ConfigRegBlock
  system_ready      ‾‾‾‾‾\_____________________  (cleared)
  inbound_enable    ‾‾‾‾‾\_____________________  (cleared)
  outbound_enable   ‾‾‾‾‾\_____________________  (cleared)

isolate_req         _______________________\_____  (deassert)
wait(SC_ZERO_TIME)  ───────────────────────────►

ConfigRegBlock
  system_ready      ___________________________  (NOT restored)
  inbound_enable    ___________________________  (NOT restored)
  outbound_enable   ___________________________  (NOT restored)
```

### A.3 Cold Reset Cycle

```
cold_reset_n        ‾‾‾‾‾\_____/‾‾‾‾‾‾‾‾‾‾‾‾‾
wait(SC_ZERO_TIME)  ───────►      ───────────►

ClockResetControl
  pcie_sii_reset_   ‾‾‾‾‾\_____/‾‾‾‾‾‾‾‾‾‾‾‾‾
  pcie_reset_ctrl_  ‾‾‾‾‾\_____/‾‾‾‾‾‾‾‾‾‾‾‾‾

Config Registers    ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾  (preserved)
TLB Entries         ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾  (preserved)
system_ready        ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾  (preserved)
```

---

## Appendix B: Address Decode Examples

### B.1 Inbound TLB Translation (TLB Sys In0)

```
Input Address:  0x4000000000003ABC
Route bits:     0x4 [63:60]           → TLB Sys In0
Index bits:     0b000000 [19:14]     → Entry 0
Offset bits:    0x3ABC [13:0]        → 14-bit page offset

Entry 0 Lookup:
    valid = true
    addr  = 0x000000FEED00000  (52 bits, [63:12])

Translation:
    translated_addr = (0x000000FEED00000 << 12) | 0x3ABC
                    = 0x000000FEED003ABC
```

### B.2 Status Register Access

```
Input Address:  0xE000000000000000
Route bits:     0xE [63:60]

Route check:
    is_status_register_access(0xE000000000000000, is_read=true)
    → route_bits == 0xE && is_read && system_ready_ → TRUE

Action:
    Return 32-bit status value:
        status[0] = system_ready (1)
        status[1] = pcie_outbound_app_enable (1)
        status[2] = pcie_inbound_app_enable (1)
        status[31:3] = 0

    Response = 0x00000007
```

### B.3 SMN Config Write (Address Passthrough Issue)

```
Input Address:  0x18800000 (MSI Relay Config base)
Expected:       Offset 0x0000 (relative to base)
Actual:         Offset 0x18800000 (full address passed)

SMN-IO Switch:
    Detects addr == 0x18800000
    Routes to msi_relay_->process_csr_access(trans, delay)

MSI Relay:
    Receives trans.get_address() = 0x18800000
    Expects offset < 4096 (4KB config space)
    Check: 0x18800000 >= 4096 → FAIL
    Result: TLM_ADDRESS_ERROR_RESPONSE

Consequence: Config write rejected
```

---

## Appendix C: Test Coverage Summary

**Test Suite:** 76 test cases, 251 assertions, 0 failures

**E2E Tests (41):**
- Inbound TLB paths (10 tests)
- Outbound TLB paths (9 tests)
- Bypass paths (2 tests)
- MSI relay (8 tests)
- SII tracking (4 tests)
- Switch routing (4 tests)
- Power/isolation (2 tests)
- Integration (2 tests)

**Directed Tests (35):**
- TLB unit tests (8 tests)
- MSI relay unit tests (5 tests)
- Switch routing unit tests (7 tests)
- Config register unit tests (3 tests)
- SII unit tests (6 tests)
- Signal propagation tests (3 tests)
- Reset sequence tests (2 tests)
- Integration stress test (1 test)

**Coverage Metrics:**
- **Functional Coverage:** 100% of testable features
- **Route Coverage:** All 16 route values exercised (0x0-0xF)
- **TLB Boundary Coverage:** Entry 0, 1, 63 tested
- **Error Path Coverage:** DECERR, isolation blocking, invalid TLB entries
- **Signal Coverage:** All input/output signals exercised except 3 (covered by new tests)

---

**Document Control:**
- **Version:** 1.0
- **Date:** February 5, 2026
- **Status:** Released
- **Next Review:** Upon DUT architecture changes or test plan updates

---
