# VDK Platform Guide: Linux-Booting Keraunos PCIe Tile

**Version:** 1.0  
**Date:** March 26, 2026  
**Author:** System Architecture Team

---

## 1. Introduction

This guide documents the final validated Synopsys Virtualizer (VDK) platform that boots RISC-V Linux on the host chiplet, enumerates a PCIe Endpoint connected to the Keraunos PCIe Tile, and enables data transfer from the host to memory attached to the tile's `noc_n_initiator` port.

The reference implementation is at `/localdev/pdroy/ruchika_ws/Keraunos_PCIE_Tile`.

### 1.1 What This Platform Demonstrates

- Full Linux boot on a RISC-V virtual CPU (TT_Rocket_LT) via OpenSBI
- PCIe link-up between a DesignWare Root Complex (host) and Endpoint (device)
- Linux kernel PCIe enumeration using the `snps,dw-pcie` driver
- BAR0 mapping and MMIO access from Linux userspace
- End-to-end data transfer: Host → RC → EP → PCIE_TILE → `noc_n_initiator` → Target_Memory

---

## 2. Platform Architecture

### 2.1 Two-Chiplet Topology

The VDK instantiates two chiplet groups with a direct PCIe link:

```{mermaid}
graph TB
    subgraph host["Host_Chiplet"]
        direction TB
        H_CPU["TT_Rocket_LT<br/>RISC-V rv64imac<br/>Runs Linux"]
        H_SMC["SMC (CPU + Peripherals)"]
        H_DRAM["DRAM<br/>256 MB @ 0x80000000"]
        H_UART["UART @ 0xC000A000"]
        H_PLIC["PLIC @ 0xC4000000"]
        H_RC["PCIE_RC<br/>DesignWare PCIe 2.0<br/>Root Complex"]
        H_MAP["SharedMemoryMap"]
        H_CLK["CLK_GEN"]
        H_RST["RST_GEN"]

        H_CPU --> H_SMC
        H_SMC --> H_MAP
        H_MAP --> H_DRAM
        H_MAP --> H_UART
        H_MAP --> H_PLIC
        H_MAP -->|DBI 0x44000000<br/>4 MB| H_RC
        H_MAP -->|AXI 0x70000000<br/>256 MB| H_RC
    end

    subgraph device["Keraunos_PCIE_Chiplet"]
        direction TB
        D_CPU["TT_Rocket_LT<br/>RISC-V<br/>pcie_bringup firmware"]
        D_SMC["SMC_Configure"]
        D_EP["PCIe_EP<br/>DesignWare PCIe 2.0<br/>Endpoint"]
        D_TILE["PCIE_TILE<br/>Keraunos PCIe Tile"]
        D_MEM["Target_Memory<br/>16 MB @ 0x0"]
        D_MAP["SharedMemoryMap"]
        D_CLK["CLK_GEN"]
        D_RST["RST_GEN"]

        D_CPU --> D_SMC
        D_SMC --> D_MAP
        D_EP -->|BusMaster| D_TILE
        D_TILE -->|pcie_controller_initiator| D_EP
        D_TILE -->|noc_n_initiator @ 0x0| D_MAP
        D_TILE -->|smn_n_initiator @ 0x0| D_MAP
        D_MAP --> D_MEM
    end

    H_RC <-->|"PCIMem ↔ PCIMem_Slave<br/>Direct PCIe Link"| D_EP
    H_RC -.->|"msi_ctrl_int → irqS[11]"| H_SMC

    style H_CPU fill:#e3f2fd
    style H_RC fill:#ffcccc,stroke:#c00
    style D_EP fill:#fff3e0
    style D_TILE fill:#c8e6c9,stroke:#2e7d32
    style D_MEM fill:#ffe6cc
    style H_DRAM fill:#e1ffe1
```

### 2.2 PCIe Link Wiring

The PCIe link uses direct peer-to-peer binding (no PCIeSwitch):

| Source | Destination | Purpose |
|--------|-------------|---------|
| `PCIE_RC.PCIMem` (master) | `PCIe_EP.PCIMem_Slave` (slave) | Downstream TLPs (host → device) |
| `PCIe_EP.PCIMem` (master) | `PCIE_RC.PCIMem_Slave` (slave) | Upstream TLPs (device → host) |
| `PCIE_RC.BusMaster` | `Host_Chiplet.SharedMemoryMap` | RC DMA to host memory |
| `PCIe_EP.BusMaster` | `PCIE_TILE.pcie_controller_target` | EP delivers inbound TLPs to tile |
| `PCIE_TILE.pcie_controller_initiator` | `PCIe_EP.AXI_Slave` | Tile sends outbound TLPs to EP |

### 2.3 PCIE_TILE Interface Connections

The Keraunos PCIe Tile connects to the chiplet's SharedMemoryMap and EP:

| Tile Port | Direction | Connected To | Address / Offset |
|-----------|-----------|-------------|------------------|
| `pcie_controller_target` | Target (in) | EP BusMaster | Inbound TLPs from host |
| `pcie_controller_initiator` | Initiator (out) | EP AXI_Slave | Outbound TLPs to host |
| `noc_n_initiator` | Initiator (out) | SharedMemoryMap (left) | Start 0x0 |
| `noc_n_target` | Target (in) | SharedMemoryMap (right) | 0x44400000, 16 MB |
| `smn_n_initiator` | Initiator (out) | SharedMemoryMap (left) | Start 0x0 |
| `smn_n_target` | Target (in) | SharedMemoryMap (right) | 0x18000000, 8 MB |

---

## 3. Memory Maps

### 3.1 Host_Chiplet CPU Address Space

| Address | Size | Target | Description |
|---------|------|--------|-------------|
| `0x02000000` | 64 KB | CLINT | Machine timer and software interrupts |
| `0x44000000` | 4 MB | PCIE_RC DBI | RC configuration registers (DesignWare DBI) |
| `0x44300000` | 128 KB | PCIE_RC ATU | iATU outbound/inbound (via DBI CS2 range) |
| `0x70000000` | 256 MB | PCIE_RC AXI_Slave | PCIe outbound window |
| `0x70000000–0x70FFFFFF` | 16 MB | — Config | Type 0/1 Configuration TLPs (iATU-translated) |
| `0x71000000–0x7FFFFFFF` | 240 MB | — MEM | Memory Read/Write TLPs to EP BARs |
| `0x80000000` | 256 MB | DRAM | Host main memory (OpenSBI + Linux + initramfs) |
| `0xC000A000` | 256 B | UART | DW APB UART, 115.2 MHz clock, 7.2 Mbaud |
| `0xC4000000` | 2 MB | PLIC | RISC-V PLIC, 64 interrupt sources |

### 3.2 Keraunos_PCIE_Chiplet SharedMemoryMap Decode

Configured via VPCFG `range_mappings`: `0x44000000:0x00400000:s;0x18000000:0x00800000;0x44400000:0x01000000;0x0:0x1000000`

| Address | Size | Target | Purpose |
|---------|------|--------|---------|
| `0x00000000` | 16 MB | Target_Memory (MEM) | Data memory — host-accessible via EP BAR0 |
| `0x18000000` | 8 MB | PCIE_TILE smn_n_target | SMN access into tile registers |
| `0x44000000` | 4 MB | PCIe_EP AXI_DBI | EP DBI configuration (`:s` = secure) |
| `0x44400000` | 16 MB | PCIE_TILE noc_n_target | NoC access into tile subsystem |

---

## 4. Linux Boot

### 4.1 Boot Flow

1. **Virtualizer loads images**: `fw_payload.elf` on Host_Chiplet CPU, `pcie_bringup.elf` on Device CPU
2. **OpenSBI** initializes M-mode on Host CPU at entry `0x80000000`
3. OpenSBI transitions to S-mode and jumps to the embedded Linux kernel
4. **Linux** parses the compiled device tree (`keraunos_host.dtb`)
5. Kernel initializes CLINT (timer), PLIC (interrupts), UART (console)
6. The `snps,dw-pcie` driver probes the PCIe RC at DBI `0x44000000`
7. Driver programs iATU outbound windows and scans bus 1
8. **EP is enumerated**: vendor/device ID read, BARs assigned
9. Linux mounts initramfs and launches `/init` (BusyBox shell)
10. The `pcie_xfer` application is available for PCIe data transfer

### 4.2 Device Tree Highlights

```
/ {
    compatible = "tt,smc-pcie-tile";
    cpus { cpu@0 { riscv,isa = "rv64imac"; mmu-type = "riscv,sv39"; }; };
    memory@80000000 { reg = <0x0 0x80000000 0x0 0x10000000>; };  /* 256 MB */

    soc {
        clint@2000000  { reg = <0x0 0x02000000 0x0 0x10000>; };
        plic@c4000000  { riscv,ndev = <64>; };
        uart@c000a000  { clock-frequency = <0x6DDD000>; };       /* 115.2 MHz */

        pcie@44000000 {
            compatible = "snps,dw-pcie";
            reg = <0x0 0x44000000 0x0 0x400000>,                 /* DBI */
                  <0x0 0x70000000 0x0 0x01000000>;               /* config */
            bus-range = <0x0 0x1>;
            ranges = <0x02000000 0x0 0x71000000
                      0x0 0x71000000  0x0 0x0F000000>;           /* 240 MB MEM */
            interrupts = <32>, <33>;                              /* MSI, INTx */
            num-lanes = <4>;
        };
    };

    chosen {
        bootargs = "console=hvc0 earlycon=sbi rdinit=/init
                    pci=realloc pci=assign-busses pci=noaer pcie_aspm=off";
    };
};
```

### 4.3 Key Boot Parameters

| Parameter | Value | Purpose |
|-----------|-------|---------|
| `console=hvc0` | HVC console | Uses SBI console calls (fastest in VP) |
| `earlycon=sbi` | SBI earlycon | Early console via OpenSBI ecall |
| `rdinit=/init` | Initramfs init | Uses embedded initramfs (no disk) |
| `pci=realloc` | PCI realloc | Reallocate PCI resources (no BIOS pre-assignment) |
| `pci=assign-busses` | Assign buses | Force bus number assignment |
| `pci=noaer` | Disable AER | Advanced Error Reporting not needed in VP |
| `pcie_aspm=off` | Disable ASPM | Active State Power Management not modeled |

### 4.4 Toolchain Requirements

| Component | Toolchain | ISA | Notes |
|-----------|-----------|-----|-------|
| Linux kernel | ASC GCC 14 (`riscv64-unknown-linux-gnu-`) | rv64imac | `CONFIG_FPU=n` |
| Initramfs (BusyBox) | Musl (`riscv64-unknown-linux-musl-`) | rv64imac / lp64 | Soft-float, static |
| OpenSBI | ASC GCC 14 | rv64imac | Builds `fw_payload.elf` |
| pcie_bringup (device) | Bare-metal GCC | rv64imac | No OS |

---

## 5. PCIe Enumeration and BAR Assignment

### 5.1 Enumeration Topology

```{mermaid}
graph LR
    subgraph bus0["PCI Bus 0"]
        RC["Root Complex<br/>Type 1 Header"]
    end

    subgraph bus1["PCI Bus 1"]
        EP["Endpoint<br/>Type 0 Header<br/>BAR0: 16 MB"]
    end

    RC -->|"Downstream Port"| EP

    style RC fill:#e3f2fd
    style EP fill:#fff3e0
```

| Property | Value |
|----------|-------|
| RC Bus/Dev/Fn | 00:00.0 |
| EP Bus/Dev/Fn | 01:00.0 |
| EP BAR0 size | 16 MB (`BAR0_MASK = 0xFFFFFF`) |
| EP BAR0 type | 64-bit Memory, non-prefetchable |
| Config access | iATU-translated (no ECAM hardware) |
| Host MEM window | `0x71000000–0x7FFFFFFF` |

### 5.2 iATU Configuration

The `dw-pcie` Linux driver programs iATU outbound windows:

- **Window 0 (Config)**: Maps `0x70000000` (16 MB) → Type 0/1 config TLPs to bus 1
- **Window 1 (MEM)**: Maps `0x71000000` (240 MB) → Memory TLPs targeting EP BARs

Linux assigns BAR0 an address within the `0x71000000–0x7FFFFFFF` MEM window. The physical address is visible in sysfs at `/sys/bus/pci/devices/0000:01:00.0/resource`.

---

## 6. Data Transfer Application (pcie_xfer)

### 6.1 Purpose

`pcie_xfer` is a Linux userspace C application that maps EP BAR0 into process address space (via sysfs `resource0` or `/dev/mem`) and performs MMIO read/write operations. Each operation traverses the full data path to `Target_Memory` on the device chiplet.

### 6.2 Data Path

```
Host CPU (MMIO store)
  → RC AXI_Slave (0x70000000 window)
    → iATU translates to Memory Write TLP
      → RC.PCIMem → PCIe link → EP.PCIMem_Slave
        → EP.BusMaster
          → PCIE_TILE.pcie_controller_target
            → Internal tile fabric
              → noc_n_initiator
                → SharedMemoryMap decode
                  → Target_Memory @ 0x0 (16 MB)
```

### 6.3 Supported Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| Write | `write <offset> <value>` | Single 32-bit write at BAR0 + offset |
| Read | `read <offset>` | Single 32-bit read at BAR0 + offset |
| Fill | `fill <offset> <count> <val>` | Fill `count` dwords with a constant value |
| Dump | `dump <offset> <count>` | Hex dump `count` dwords from BAR0 |
| Pattern | `pattern <offset> <count>` | Write `0xA0000000 | i` pattern and verify |
| Burst | `burst <offset> <count>` | Timed burst write, reports throughput |
| Verify | `verify <offset> <count> <val>` | Verify memory matches expected value |
| Send | `send <offset> <file>` | Write binary file contents to BAR0 |

### 6.4 EP Auto-Detection

The application walks `/sys/bus/pci/devices/` to find the first non-bridge device (class != `0x0604xx` or `0x0600xx`), reads BAR0 physical address and size from the sysfs `resource` file, and maps it via `resource0` (falling back to `/dev/mem` if needed).

### 6.5 Building pcie_xfer

Cross-compile for the host initramfs with the musl toolchain:

```bash
riscv64-unknown-linux-musl-gcc -O2 -static -o pcie_xfer pcie_xfer.c
```

The static binary is included in the initramfs CPIO archive.

---

## 7. Bare-Metal Test (pcie_e2e_test)

An alternative bare-metal test (`pcie_e2e_test.c`) runs on the Host_Chiplet without Linux. It provides:

- Trap-safe MMIO operations (catch and report access faults)
- RC DBI register dump (Vendor/Device ID, Command/Status, BARs, Capabilities)
- Port Logic register inspection
- iATU programming and verification
- End-to-end data transfer test through the PCIe link
- Detailed topology verification (Bus 0 RC, Bus 1 EP)

This test is useful for pre-Linux validation of the PCIe link and tile connectivity.

---

## 8. VP Configuration Reference

### 8.1 Available Configurations

| Name | Active Config Path | Description |
|------|-------------------|-------------|
| **default** | `vpconfigs/default/default.vpcfg` | Full Linux with standard kernel, quantum 6000 ps |
| **mini_riscv64_linux** | `vpconfigs/mini_riscv64_linux/mini_riscv64_linux.vpcfg` | Mini Linux for fast boot, quantum 1000 ps |

Set the active config in `snps.vpproject` → `activeConfig` attribute.

### 8.2 Critical VPCFG Parameters

**Host_Chiplet:**

| Parameter | Value | Purpose |
|-----------|-------|---------|
| `RST_GEN.active_at_start` | `true` | Start host CPU immediately |
| `PCIE_RC.SHARED_DBI_ENABLED` | `false` | Separate DBI window |
| `PCIE_RC.cc_pipe_clk_Override` | 250 | PIPE clock 250 MHz |
| `PCIE_RC.cc_*_Override` | 100 | AXI/DBI/aux clocks 100 MHz |
| `initial_image` | `fw_payload.elf` (empty load addr) | OpenSBI + Linux |
| `quantum_value` | 6000 (default) / 1000 (mini) | Temporal decoupling quantum |
| `UART_CLK` | 115200000 | 115.2 MHz UART clock |

**Keraunos_PCIE_Chiplet:**

| Parameter | Value | Purpose |
|-----------|-------|---------|
| `RST_GEN.active_at_start` | `true` | Start device CPU immediately |
| `PCIe_EP.SHARED_DBI_ENABLED` | `false` | Separate DBI window |
| `PCIe_EP.cc_pipe_clk_Override` | 250 | PIPE clock 250 MHz |
| `initial_image` | `pcie_bringup.elf` | Bare-metal EP init firmware |
| `SharedMemoryMap_intf.range_mappings` | See Section 3.2 | Chiplet address decode |

### 8.3 Image Loading

**Critical**: The host `initial_image` uses the format `{path/fw_payload.elf,,,image+symbols,}` with **empty load address fields**. The Virtualizer applies ELF program header addresses (entry at `0x80000000`). **Never** set load address to `0x0` — this breaks OpenSBI execution.

---

## 9. Sideband Signal Mapping

### 9.1 EP ↔ PCIE_TILE Sideband

| VDK Connection Source | VDK Connection Sink | Description |
|-----------------------|---------------------|-------------|
| `PCIe_EP.edma_int` / `edma_int_*` | `PCIE_TILE.pcie_misc_int` | DMA interrupts |
| `PCIe_EP.pcie_parc_int` | `PCIE_TILE.pcie_ras_error` | Parity/RAS error |
| `PCIe_EP.lbc_cii_hv` | `PCIE_TILE.pcie_cii_hv` | CII header valid |
| `PCIe_EP.lbc_cii_hdr_type` | `PCIE_TILE.pcie_cii_hdr_type` | CII header type |
| `PCIe_EP.lbc_cii_hdr_addr` | `PCIE_TILE.pcie_cii_hdr_addr` | CII header address |
| `PCIe_EP.cfg_flr_pf_active_x[0]` | `PCIE_TILE.pcie_flr_request` | Function Level Reset |

### 9.2 Clocks and Resets

| Source | Destination | Notes |
|--------|-------------|-------|
| `Chiplet.Infra.CLK_GEN` | `PCIE_TILE`, `PCIe_EP` | System clock |
| `Chiplet.Infra.RST_GEN` | `PCIE_TILE`, `PCIe_EP` | System reset |
| `Host.Infra.CLK_GEN` | `PCIE_RC` | Host clock |
| `Host.Infra.RST_GEN` | `PCIE_RC` | Host reset |

### 9.3 MSI Interrupt Path

```
PCIE_RC.msi_ctrl_int → Host_Chiplet.SMC.irqS[11] → PLIC IRQ → Linux ISR
```

---

## 10. Troubleshooting

### 10.1 Host Crashes or Nonsense PC

1. Verify `fw_payload.elf` uses ELF-header load addresses (not forced `0x0`)
2. Check `readelf -l fw_payload.elf` — VirtAddr should show `0x8000xxxx`
3. Ensure kernel is built with `CONFIG_FPU=n` for rv64imac CPU
4. Verify initramfs binaries are musl-compiled for rv64imac (no FPU instructions)

### 10.2 PCIe EP Not Detected

1. Verify `pcie_bringup.elf` is loaded on device CPU and runs to completion
2. Check that device `RST_GEN.active_at_start = true`
3. Confirm direct PCIe link wiring (RC.PCIMem ↔ EP.PCIMem_Slave)
4. Verify DT `bus-range = <0x0 0x1>` and bootargs include `pci=assign-busses`

### 10.3 BAR0 Access Returns All-Ones

1. EP may not have BAR0 properly sized — check `pcie_bringup` programs `BAR0_MASK`
2. iATU outbound window may not cover the assigned BAR address
3. Verify chiplet SharedMemoryMap decode includes `0x0:0x1000000` for Target_Memory

### 10.4 Data Mismatch on Read-Back

1. Check that `noc_n_initiator` is connected to SharedMemoryMap at start 0x0
2. Verify Target_Memory size (16 MB) covers the access offset
3. Confirm byte-enable and data width compatibility across the path

---

## 11. Related Documents

| Document | Description |
|----------|-------------|
| [Keraunos System Architecture](Keraunos_System_Architecture.md) | Full system architecture including chiplet ecosystem |
| [Keraunos PCIe Tile HLD](Keraunos_PCIe_Tile_HLD.md) | High-level design of the PCIe Tile |
| [SystemC Design Document](Keraunos_PCIE_Tile_SystemC_Design_Document.md) | Detailed SystemC implementation |
| [Test Plan](Keraunos_PCIE_Tile_Testplan.md) | Verification test plan |

---

**Document Control:**
- **Classification:** Internal Use Only
- **Distribution:** Keraunos Project Team, VDK Integration Team

---

**End of Document**
