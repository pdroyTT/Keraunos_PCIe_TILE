# Keraunos System Architecture
## PCIe Tile Integration in Keraunos-E100 Chiplet Ecosystem

**Version:** 1.0  
**Date:** February 10, 2026  
**Author:** System Architecture Team

---

## Executive Summary

This document describes the system-level architecture of the Keraunos-E100 chiplet ecosystem and details how the Keraunos PCIe Tile integrates into the larger Grendel multi-chiplet architecture. The Keraunos PCIe Tile serves as a critical I/O interface, enabling host connectivity and system management while interfacing with the on-chip Network-on-Chip (NOC) infrastructure.

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Keraunos-E100 Chiplet Architecture](#2-keraunos-e100-chiplet-architecture)
3. [PCIe Tile Position in the System](#3-pcie-tile-position-in-the-system)
4. [Connectivity Architecture](#4-connectivity-architecture)
5. [Data Flow Paths](#5-data-flow-paths)
6. [Address Space Integration](#6-address-space-integration)
7. [System Use Cases](#7-system-use-cases)
8. [Appendices](#8-appendices)

---

## 1. System Overview

### 1.1 Grendel Chiplet Ecosystem

The Grendel chiplet ecosystem is a multi-chiplet heterogeneous computing platform designed for high-performance AI/ML workloads. The ecosystem consists of:

- **Quasar Chiplets:** Compute chiplets containing AI/ML processing cores
- **Mimir Chiplets:** Memory chiplets with GDDR interfaces
- **Athena Chiplets:** Specialized compute chiplets
- **Keraunos-E100 Chiplets:** I/O interface chiplets for high-speed connectivity

### 1.2 Keraunos-E100 Role

Keraunos-E100 is the I/O interface chiplet family in the Grendel ecosystem, providing:

- **Glueless scale-out** connectivity via 400G/800G Ethernet (Quasar-to-Quasar across packages)
- **Host connectivity** via PCIe Gen5 (x16)
- **Die-to-die (D2D)** connectivity within the package using BoW (Bridge-of-Wire) technology
- **System management** capabilities via integrated SMC (System Management Controller)

```mermaid
graph TB
    subgraph "Grendel Package"
        QUASAR[Quasar Chiplets<br/>AI/ML Compute]
        MIMIR[Mimir Chiplets<br/>GDDR Memory]
        KERAUNOS[Keraunos-E100<br/>I/O Chiplet]
        
        QUASAR <-->|D2D BoW| KERAUNOS
        MIMIR <-->|D2D BoW| QUASAR
    end
    
    HOST[Host System<br/>x86/ARM Server] <-->|PCIe Gen5 x16| KERAUNOS
    REMOTE[Remote Grendel<br/>Package] <-->|400G/800G<br/>Ethernet| KERAUNOS
    
    style KERAUNOS fill:#e1f5ff
    style QUASAR fill:#ffe1e1
    style MIMIR fill:#e1ffe1
    style HOST fill:#fff4e1
    style REMOTE fill:#f0e1ff
```

---

## 2. Keraunos-E100 Chiplet Architecture

### 2.1 High-Level Block Diagram

The Keraunos-E100 chiplet contains the following major subsystems:

```mermaid
graph TB
    subgraph "Keraunos-E100 Chiplet"
        subgraph "Chiplet Harness"
            SMC[SMC<br/>System Management<br/>Controller]
            SEP[SEP<br/>Security Engine<br/>Processor]
            SMU[SMU<br/>System Management<br/>Unit]
        end
        
        subgraph "PCIe Subsystem"
            PCIE_TILE[PCIe Tile<br/>PCIe Gen5 x16<br/>Controller]
            PCIE_SERDES[PCIe SerDes<br/>PHY]
        end
        
        subgraph "HSIO Tiles x2"
            CCE0[CCE 0<br/>Keraunos Compute<br/>Engine]
            CCE1[CCE 1<br/>Keraunos Compute<br/>Engine]
            ETH_CTRL0[TT Ethernet<br/>Controller 0]
            ETH_CTRL1[TT Ethernet<br/>Controller 1]
            MAC0[MAC/PCS 0<br/>800G]
            MAC1[MAC/PCS 1<br/>800G]
            SRAM[HSIO SRAM<br/>8MB]
            HSIO_FABRIC[HSIO Fabric<br/>AXI Crossbar]
        end
        
        subgraph "NOC Infrastructure"
            SMN[SMN<br/>System Management<br/>Network]
            QNP[QNP Mesh<br/>NOC-N]
            D2D[D2D Tiles x5<br/>Die-to-Die<br/>Interfaces]
        end
        
        PCIE_TILE <--> SMN
        PCIE_TILE <--> QNP
        
        CCE0 <--> HSIO_FABRIC
        CCE1 <--> HSIO_FABRIC
        ETH_CTRL0 <--> HSIO_FABRIC
        ETH_CTRL1 <--> HSIO_FABRIC
        HSIO_FABRIC <--> SRAM
        HSIO_FABRIC <--> SMN
        HSIO_FABRIC <--> QNP
        
        ETH_CTRL0 <--> MAC0
        ETH_CTRL1 <--> MAC1
        
        SMN <--> SMC
        SMN <--> SEP
        SMN <--> D2D
        
        QNP <--> D2D
        
        MAC0 -.->|Ethernet<br/>SerDes| EXT_ETH0[External<br/>Ethernet 0]
        MAC1 -.->|Ethernet<br/>SerDes| EXT_ETH1[External<br/>Ethernet 1]
        
        PCIE_SERDES -.->|PCIe<br/>Lanes| EXT_PCIE[External<br/>PCIe Host]
        PCIE_TILE <--> PCIE_SERDES
        
        D2D -.->|BoW| EXT_D2D[Quasar/Mimir<br/>Chiplets]
    end
    
    style PCIE_TILE fill:#ffcccc,stroke:#ff0000,stroke-width:3px
    style SMC fill:#cce5ff
    style SEP fill:#cce5ff
    style CCE0 fill:#ffe6cc
    style CCE1 fill:#ffe6cc
    style SMN fill:#e6ccff
    style QNP fill:#e6ccff
    style HSIO_FABRIC fill:#d9f7d9
```

### 2.2 Key Subsystems

#### 2.2.1 Chiplet Harness
- **SMC (System Management Controller):** 4-core RISC-V processor (Rocket core) running at 800 MHz
- **SEP (Security Engine Processor):** Handles secure boot, attestation, and access filtering
- **SMU (System Management Unit):** Clock generation (CGM PLLs), power management, reset sequencing

#### 2.2.2 PCIe Subsystem
- **PCIe Tile:** Contains PCIe Gen5 controller, TLB translation engines, configuration registers
- **PCIe SerDes:** Physical layer (PHY) for PCIe Gen5 x16 connectivity

#### 2.2.3 HSIO (High-Speed I/O) Tiles
- **CCE (Keraunos Compute Engine):** DMA engines, DMRISC cores, data forwarding logic
- **TT Ethernet Controller:** TX/RX queue controllers, packet processing, flow control
- **MAC/PCS:** 800G Ethernet MAC and Physical Coding Sublayer (OmegaCore IP from AlphaWave)
- **SRAM:** 8MB high-speed SRAM for packet buffering and data staging
- **HSIO Fabric:** AXI-based crossbar interconnect

#### 2.2.4 NOC Infrastructure
- **SMN (System Management Network):** Carries control, configuration, and low-bandwidth traffic
- **QNP Mesh (NOC-N):** High-bandwidth data fabric for payload transfer (1.5 GHz @ TT corner)
- **D2D (Die-to-Die):** 5 BoW interfaces @ 2 GHz for chiplet-to-chiplet connectivity

---

## 3. PCIe Tile Position in the System

### 3.1 PCIe Tile Overview

The **Keraunos PCIe Tile** developed in this project is a SystemC/TLM-2.0 model representing the PCIe subsystem of the Keraunos-E100 chiplet. It provides:

1. **Host Interface:** PCIe Gen5 x16 connectivity to the host CPU
2. **Internal Routing:** Bidirectional routing between PCIe, NOC-N (QNP), and SMN
3. **Address Translation:** TLB-based address mapping between PCIe address space and system address space
4. **Configuration Interface:** SMN-accessible configuration registers for TLBs, MSI relay, and error handling

### 3.2 Architectural Position

```mermaid
graph LR
    subgraph "Host System"
        CPU[Host CPU<br/>x86/ARM]
        DRAM[Host DRAM]
    end
    
    subgraph "Keraunos-E100 Chiplet"
        subgraph "PCIe Tile"
            PCIE_CTRL[PCIe Controller]
            TLB_IN[Inbound TLB<br/>8 Entries]
            TLB_OUT[Outbound TLB<br/>8 Entries]
            PCIE_SWITCH[PCIe-SMN-IO<br/>Switch]
            MSI_RELAY[MSI Relay]
        end
        
        SMN_NET[SMN Network<br/>System Management]
        QNP_NET[QNP/NOC-N<br/>High-BW Data]
        
        HSIO_TILES[HSIO Tiles<br/>Ethernet CCE]
        D2D_LINKS[D2D Links<br/>to Quasar]
    end
    
    CPU <-->|PCIe TLP| PCIE_CTRL
    PCIE_CTRL <-->|Inbound| TLB_IN
    PCIE_CTRL <-->|Outbound| TLB_OUT
    TLB_IN --> PCIE_SWITCH
    PCIE_SWITCH --> QNP_NET
    PCIE_SWITCH --> SMN_NET
    
    TLB_OUT <--> QNP_NET
    TLB_OUT <--> SMN_NET
    
    SMN_NET <--> PCIE_CTRL
    SMN_NET <--> HSIO_TILES
    QNP_NET <--> HSIO_TILES
    QNP_NET <--> D2D_LINKS
    
    MSI_RELAY --> PCIE_CTRL
    SMN_NET --> MSI_RELAY
    
    style PCIE_CTRL fill:#ffcccc,stroke:#ff0000,stroke-width:3px
    style TLB_IN fill:#ffe6cc
    style TLB_OUT fill:#ffe6cc
    style PCIE_SWITCH fill:#d9f7d9
    style SMN_NET fill:#e6ccff
    style QNP_NET fill:#cce5ff
```

### 3.3 Key Interfaces

| Interface | Protocol | Width | Purpose |
|-----------|----------|-------|---------|
| `pcie_inbound` | TLM-2.0 Target | 64-bit | Receives PCIe Memory Read/Write from host |
| `noc_n_initiator` | TLM-2.0 Initiator | 64-bit | Forwards inbound PCIe traffic to NOC after TLB translation |
| `smn_n_initiator` | TLM-2.0 Initiator | 64-bit | Forwards bypass/system traffic to SMN |
| `noc_n_outbound` | TLM-2.0 Target | 64-bit | Receives outbound NOC traffic destined for PCIe |
| `smn_outbound` | TLM-2.0 Target | 64-bit | Receives outbound SMN traffic destined for PCIe |
| `pcie_controller_initiator` | TLM-2.0 Initiator | 64-bit | Sends outbound transactions to PCIe controller |
| `smn_config` | TLM-2.0 Target | 64-bit | SMN access to PCIe Tile configuration registers |

---

## 4. Connectivity Architecture

### 4.1 Inbound Data Path (Host → Chip)

**Use Case:** Host CPU writes data to Quasar compute cores or Mimir memory.

```mermaid
sequenceDiagram
    participant Host as Host CPU
    participant PCIe as PCIe Tile
    participant TLB as Inbound TLB
    participant Switch as PCIe-SMN-IO<br/>Switch
    participant NOC as NOC-N (QNP)
    participant Quasar as Quasar Chiplet

    Host->>PCIe: PCIe Memory Write<br/>(Host Address)
    PCIe->>TLB: Lookup Translation<br/>(Host Addr → System Addr)
    TLB-->>PCIe: System Address<br/>(NOC Address)
    PCIe->>Switch: Forward to Switch
    Switch->>Switch: Route Decision<br/>(NOC or SMN?)
    
    alt NOC-bound traffic
        Switch->>NOC: Forward to NOC-N
        NOC->>Quasar: D2D BoW to Quasar
        Quasar-->>NOC: Write Response
        NOC-->>Switch: Response
    else SMN-bound traffic
        Switch->>SMN: Forward to SMN
        SMN-->>Switch: Response
    end
    
    Switch-->>PCIe: Completion
    PCIe-->>Host: PCIe Completion TLP
```

**Key Steps:**
1. Host initiates PCIe Memory Write targeting Keraunos BAR (Base Address Register)
2. PCIe Tile receives transaction via `pcie_inbound` socket
3. Inbound TLB translates host address to system address space
4. PCIe-SMN-IO Switch routes based on address:
   - **0x0000\_0000\_0000 - 0x0000\_FFFF\_FFFF:** NOC-bound (via `noc_n_initiator`)
   - **0x1000\_0000\_0000 - 0x1FFF\_FFFF\_FFFF:** SMN-bound (via `smn_n_initiator`)
5. Transaction forwarded to NOC-N or SMN
6. NOC-N routes via D2D links to destination Quasar/Mimir chiplet
7. Response traverses back through the same path

### 4.2 Outbound Data Path (Chip → Host)

**Use Case:** Quasar compute cores send results back to host DRAM or trigger MSI interrupts.

```mermaid
sequenceDiagram
    participant Quasar as Quasar Chiplet
    participant NOC as NOC-N (QNP)
    participant PCIe as PCIe Tile
    participant TLB as Outbound TLB
    participant PCIe_Ctrl as PCIe Controller
    participant Host as Host CPU/DRAM

    Quasar->>NOC: Write to PCIe<br/>(System Address)
    NOC->>PCIe: Forward via noc_n_outbound
    PCIe->>TLB: Outbound TLB Lookup<br/>(System Addr → Host Addr)
    TLB-->>PCIe: Host Address
    PCIe->>PCIe_Ctrl: Forward to PCIe Controller
    PCIe_Ctrl->>Host: PCIe Memory Write TLP<br/>(Host Address)
    Host-->>PCIe_Ctrl: PCIe Completion
    PCIe_Ctrl-->>PCIe: Response
    PCIe-->>NOC: Write Response
    NOC-->>Quasar: Completion
```

**Key Steps:**
1. Quasar initiates write targeting PCIe address range (typically host DRAM)
2. NOC-N routes to Keraunos PCIe Tile via `noc_n_outbound` socket
3. Outbound TLB translates system address back to host physical address
4. PCIe Controller (`pcie_controller_initiator`) generates PCIe Memory Write TLP
5. Transaction sent over PCIe link to host
6. Host DRAM responds with completion
7. Response propagates back through PCIe Tile → NOC → Quasar

### 4.3 Configuration Path (SMN → PCIe Tile Registers)

**Use Case:** SMC programs PCIe Tile TLBs, enables MSI relay, or reads error status.

```mermaid
sequenceDiagram
    participant SMC as SMC (RISC-V)
    participant SMN as SMN Network
    participant PCIe as PCIe Tile<br/>Config Registers

    SMC->>SMN: SMN Write<br/>(Config Addr)
    SMN->>PCIe: Forward to smn_config
    PCIe->>PCIe: Update TLB Entry<br/>or MSI Config
    PCIe-->>SMN: Response
    SMN-->>SMC: Write Complete
```

**Addressable Registers (via SMN):**
- **0x1804\_0000 - 0x1804\_07FF:** Inbound TLB configurations (8 entries)
- **0x1804\_0800 - 0x1804\_0FFF:** Outbound TLB configurations (8 entries)
- **0x1800\_0000 - 0x1800\_0FFF:** MSI Relay registers
- **0x1802\_0000 - 0x1802\_0FFF:** PCIe error status and control

### 4.4 MSI Interrupt Path (Chip → Host)

**Use Case:** Ethernet controller or Quasar triggers interrupt to host driver.

```mermaid
sequenceDiagram
    participant HSIO as HSIO Tile<br/>(Ethernet)
    participant SMN as SMN Network
    participant MSI as MSI Relay<br/>(PCIe Tile)
    participant PCIe as PCIe Controller
    participant Host as Host CPU

    HSIO->>SMN: Write to MSI Relay<br/>(Trigger Interrupt)
    SMN->>MSI: Forward MSI Trigger
    MSI->>MSI: Translate to<br/>PCIe MSI-X Format
    MSI->>PCIe: Generate MSI-X TLP
    PCIe->>Host: PCIe MSI-X Write<br/>(Interrupt)
    Host->>Host: Invoke ISR<br/>(Interrupt Handler)
```

---

## 5. Data Flow Paths

### 5.1 End-to-End Data Flow Example: Host DMA to Quasar

**Scenario:** Host writes 4KB of neural network weights to Quasar L1 memory.

```mermaid
graph LR
    subgraph "Host System"
        A[Host DMA Engine]
    end
    
    subgraph "Keraunos PCIe Tile"
        B[PCIe Inbound]
        C[Inbound TLB<br/>App 0]
        D[PCIe-SMN-IO<br/>Switch]
        E[NOC-N Initiator]
    end
    
    subgraph "NOC Infrastructure"
        F[QNP Mesh]
        G[D2D Tile]
    end
    
    subgraph "Quasar Chiplet"
        H[NOC Router]
        I[Tensix Core<br/>L1 Memory]
    end
    
    A -->|PCIe Gen5<br/>Memory Write| B
    B -->|TLM Target| C
    C -->|Translate<br/>0x8000_0000 →<br/>0x0_4000_0000| D
    D -->|Route to NOC| E
    E -->|TLM Initiator| F
    F -->|QNP @ 1.5GHz| G
    G -->|BoW D2D| H
    H -->|Local NOC| I
    
    style B fill:#ffcccc
    style C fill:#ffe6cc
    style D fill:#d9f7d9
    style E fill:#cce5ff
    style F fill:#cce5ff
    style G fill:#e6ccff
    style I fill:#ffe6cc
```

**Address Translation:**
- **Host Address:** `0x8000_0000` (PCIe BAR + offset)
- **Inbound TLB Lookup:** Maps application region 0 → NOC address
- **System Address:** `0x0000_0000_4000_0000` (Quasar chiplet, NOC coordinates, L1 offset)
- **Physical Routing:** QNP mesh routes to D2D tile 2 → Quasar chiplet ID 1 → Tensix core (4,5)

### 5.2 Multi-Hop Data Flow: Quasar → PCIe → Host → PCIe → Quasar

**Scenario:** Quasar chiplet 0 sends data to Quasar chiplet 1 in a different Grendel package via host DRAM (zero-copy).

```mermaid
graph TB
    subgraph "Package 0"
        Q0[Quasar 0]
        K0[Keraunos PCIe 0]
        Q0 -->|1. NOC Write| K0
    end
    
    H[Host DRAM<br/>Shared Buffer]
    K0 -->|2. PCIe Write| H
    
    subgraph "Package 1"
        K1[Keraunos PCIe 1]
        Q1[Quasar 1]
        K1 -->|4. NOC Write| Q1
    end
    
    H -->|3. PCIe Read| K1
    
    style Q0 fill:#ffe6cc
    style K0 fill:#ffcccc
    style H fill:#fff4e1
    style K1 fill:#ffcccc
    style Q1 fill:#ffe6cc
```

---

## 6. Address Space Integration

### 6.1 System Address Map

The Keraunos-E100 local address map is a subset of the broader Grendel system address map:

| Address Range | Target | Description |
|---------------|--------|-------------|
| `0x0000_0000_0000 - 0x0000_FFFF_FFFF` | NOC-N | Quasar/Mimir chiplets via D2D |
| `0x1000_0000_0000 - 0x1000_0FFF_FFFF` | SMN (SEP) | Security Engine Processor |
| `0x1001_0000_0000 - 0x1001_0FFF_FFFF` | SMN (SMC) | System Management Controller |
| `0x1800_0000_0000 - 0x1800_0FFF_FFFF` | SMN (MSI) | MSI Relay in PCIe Tile |
| `0x1802_0000_0000 - 0x1802_0FFF_FFFF` | SMN (PCIe Err) | PCIe Tile error registers |
| `0x1804_0000_0000 - 0x1804_0FFF_FFFF` | SMN (TLB) | PCIe Tile TLB configurations |
| `0x2000_0000_0000 - 0x2000_00FF_FFFF` | HSIO | HSIO tile 0 (CCE, Ethernet, SRAM) |
| `0x2001_0000_0000 - 0x2001_00FF_FFFF` | HSIO | HSIO tile 1 (CCE, Ethernet, SRAM) |

### 6.2 PCIe BAR (Base Address Register) Mapping

The PCIe Tile exposes multiple BARs to the host:

| BAR | Size | Type | Purpose |
|-----|------|------|---------|
| BAR0 | 256MB | Memory, 64-bit | Main data path (DMA to/from Quasar) |
| BAR2 | 16MB | Memory, 64-bit | Configuration space (SMC mailboxes, TLB programming) |
| BAR4 | 64KB | Memory, 64-bit | MSI-X table |

**BAR0 Inbound TLB Mapping Example:**
- Host writes to `BAR0 + 0x1000_0000` (256MB offset)
- Inbound TLB Entry 1 (Application region 1):
  - **Input Range:** `0x1000_0000 - 0x1FFF_FFFF` (256MB)
  - **Output Base:** `0x0000_0000_4000_0000` (NOC address for Quasar chiplet 1)
- Translated Address: `0x0000_0000_4000_0000` (sent to NOC-N)

### 6.3 Address Translation Stages

```mermaid
graph LR
    A[Host Address<br/>BAR0 + Offset]
    B[PCIe TLP<br/>Address Field]
    C[Inbound TLB<br/>Lookup]
    D[System Address<br/>NOC/SMN]
    E[NOC Routing<br/>Header]
    F[D2D Address<br/>Translation]
    G[Quasar Local<br/>Address]
    
    A --> B
    B --> C
    C --> D
    D --> E
    E --> F
    F --> G
    
    style C fill:#ffe6cc
    style D fill:#cce5ff
    style E fill:#e6ccff
```

---

## 7. System Use Cases

### 7.1 Use Case 1: Model Initialization

**Objective:** Load a 10GB large language model from host to distributed Quasar memory.

**Flow:**
1. Host driver programs PCIe Tile Inbound TLBs (8 entries for 8 memory regions)
2. Host DMA engine streams model weights via PCIe Memory Writes
3. PCIe Tile translates addresses and routes to NOC-N
4. NOC-N distributes data across multiple Quasar chiplets via D2D links
5. Quasar chiplets store weights in local L1/DRAM

**Performance:**
- PCIe Gen5 x16: ~64 GB/s theoretical, ~50 GB/s effective
- Load time: 10GB / 50 GB/s = **200ms**

### 7.2 Use Case 2: Inference Execution

**Objective:** Run inference on Quasar chiplets, stream results back to host.

**Flow:**
1. Host sends inference request descriptor via PCIe write (small payload: 256 bytes)
2. Quasar chiplets execute inference using cached model weights
3. Quasar writes results to host DRAM via outbound TLB (PCIe Memory Write)
4. Quasar triggers MSI-X interrupt via SMN → MSI Relay → PCIe
5. Host driver processes results

**Latency:**
- Request descriptor: ~1μs (PCIe TLP overhead)
- Inference execution: Variable (model-dependent)
- Result transfer (1MB): 1MB / 50 GB/s = **20μs**
- MSI interrupt latency: ~2μs

### 7.3 Use Case 3: Package-to-Package Communication

**Objective:** Enable Quasar chiplets in Package 0 to communicate with Package 1 over Ethernet.

**Flow (Keraunos Ethernet-based):**
1. Quasar in Package 0 writes data to HSIO SRAM via NOC-N
2. CCE in HSIO tile prepares Ethernet packet
3. TT Ethernet Controller sends packet via 800G Ethernet to Package 1
4. Package 1 Ethernet Controller receives packet, writes to local HSIO SRAM
5. Local NOC-N forwards data to destination Quasar

**Alternative Flow (PCIe-based, for same-host deployments):**
1. Quasar in Package 0 writes to host DRAM via PCIe Tile (outbound)
2. Package 1 PCIe Tile reads from host DRAM (inbound)
3. Forwarded to Package 1 Quasar via NOC-N

### 7.4 Use Case 4: System Management

**Objective:** SMC monitors PCIe link status and reconfigures TLBs dynamically.

**Flow:**
1. SMC reads PCIe link status registers via SMN (0x1802_0xxx)
2. Detects link degradation (Gen5 x16 → Gen5 x8)
3. SMC reprograms TLB entries to reduce traffic load
4. SMC triggers software notification via MSI-X
5. Host driver adjusts DMA batch sizes

---

## 8. Appendices

### 8.1 Acronyms and Abbreviations

| Term | Definition |
|------|------------|
| AXI | Advanced eXtensible Interface (ARM AMBA standard) |
| BAR | Base Address Register (PCIe configuration space) |
| BoW | Bridge-of-Wire (die-to-die interconnect technology) |
| CCE | Keraunos Compute Engine (DMA and packet processing) |
| D2D | Die-to-Die (chiplet interconnect interface) |
| DMA | Direct Memory Access |
| HSIO | High-Speed Input/Output (Ethernet subsystem in Keraunos) |
| ISR | Interrupt Service Routine |
| MAC | Media Access Control (Ethernet layer) |
| MSI | Message Signaled Interrupt (PCIe interrupt mechanism) |
| NOC | Network-on-Chip |
| PCS | Physical Coding Sublayer (Ethernet layer) |
| QNP | Quasar NOC Protocol (internal NOC protocol) |
| RISC-V | Reduced Instruction Set Computer - Version 5 (open ISA) |
| SCML2 | SystemC Modeling Library 2 (Synopsys verification library) |
| SEP | Security Engine Processor |
| SMC | System Management Controller |
| SMN | System Management Network (control plane NOC) |
| SMU | System Management Unit (clock/power/reset control) |
| SRAM | Static Random-Access Memory |
| TLB | Translation Lookaside Buffer (address translation cache) |
| TLP | Transaction Layer Packet (PCIe protocol) |
| TLM | Transaction-Level Modeling (SystemC abstraction) |

### 8.2 Reference Documents

1. **Keraunos-E100 Architecture Specification** (keraunos-e100-for-review.pdf, v0.9.14)
2. **Keraunos PCIe Tile High-Level Design** (Keraunos_PCIe_Tile_HLD.md, v2.0)
3. **Keraunos PCIe Tile SystemC Design Document** (Keraunos_PCIE_Tile_SystemC_Design_Document.md, v2.1)
4. **Keraunos PCIe Tile Test Plan** (Keraunos_PCIE_Tile_Testplan.md, v2.1)
5. **PCIe Base Specification 5.0** (PCI-SIG, 2019)
6. **AMBA AXI and ACE Protocol Specification** (ARM IHI 0022E)
7. **SystemC TLM-2.0 Language Reference Manual** (IEEE 1666-2011)

### 8.3 Revision History

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0 | 2026-02-10 | System Architecture Team | Initial release |

---

**Document Control:**
- **Classification:** Internal Use Only
- **Distribution:** Keraunos Project Team, Grendel Architecture Team
- **Review Cycle:** Quarterly or upon major architecture changes

---

**End of Document**
