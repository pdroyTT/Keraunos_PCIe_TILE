# SystemC IP Integration in Synopsys Virtualizer VDK
## Complete Step-by-Step Guide

**Version:** 1.0  
**Tool Version:** Synopsys Virtualizer V-2024.03  
**Date:** December 10, 2025  
**Target:** Integrating SystemC IPs (like Inbound/Outbound TLB) into VDK

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Integration Workflow](#integration-workflow)
4. [Guide Structure](#guide-structure)
5. [Quick Start](#quick-start)

---

## Overview

This guide provides comprehensive instructions for integrating custom SystemC IP blocks (such as TLB - Translation Lookaside Buffer components) into Synopsys Virtualizer Development Kit (VDK).

###What You'll Learn

- How to prepare SystemC IP for Virtualizer integration
- How to use Platform Creator to import SystemC components
- How to create TLM 2.0 interfaces for your IP
- How to package IP into a Synopsys Library
- How to integrate IP into a VDK Virtual Prototype
- How to build and simulate the integrated system

### Integration Levels

**Level 1: Basic Integration** (Quick & Simple)
- Import existing SystemC IP
- Minimal modification
- Basic TLM sockets
- Suitable for evaluation

**Level 2: Full Integration** (Production Quality)
- TLM 2.0 compliance
- Temporal decoupling
- Debug support (GDB, debugger APIs)
- Register introspection
- Full parameterization

**Level 3: Advanced Integration** (Enterprise)
- DMI (Direct Memory Interface) support
- Multi-instance support
- Software Analysis APIs
- Custom Tcl commands
- Virtual Prototype packaging

---

## Prerequisites

### Required Tools

1. **Synopsys Virtualizer V-2024.03 or later**
   - Location: `/tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03`
   - Tools needed:
     - Platform Creator (`pct.exe`)
     - VDK Creator
     - VP Explorer (`vpexplorer`)
     - SystemC Shell

2. **SystemC Library**
   - SystemC 2.3.x or later
   - TLM 2.0 library
   - Location: `/home/pdroy/systemc-local`

3. **C++ Compiler**
   - GCC 8.5+ or equivalent
   - C++17 support required

4. **Development Environment**
   - Linux (RHEL 8.x recommended)
   - Make or CMake
   - Git (for version control)

### Required Knowledge

- **Essential:**
  - SystemC basics (sc_module, SC_METHOD, SC_THREAD)
  - TLM 2.0 concepts (sockets, transactions, phases)
  - C++ programming

- **Recommended:**
  - Virtual prototyping concepts
  - Memory-mapped I/O
  - Bus protocols (AXI, AHB, etc.)
  - Hardware/Software co-design

### Files You Should Have

- **SystemC IP Source Code**
  - Header files (`.h`)
  - Implementation files (`.cpp`)
  - Test benches (optional)

- **IP Documentation**
  - Register map / address decoder
  - Interface specifications
  - Timing requirements

---

## Integration Workflow

```
┌─────────────────────────────────────────────────────────────┐
│                   INTEGRATION WORKFLOW                      │
└─────────────────────────────────────────────────────────────┘

STEP 1: Prepare SystemC IP
├─ Review IP for VDK compatibility
├─ Add TLM 2.0 sockets (if not present)
├─ Implement b_transport() method
└─ Ensure proper sc_module structure

              ↓

STEP 2: Create Platform Creator Component
├─ Import SystemC source files
├─ Configure component properties
├─ Define ports and parameters
└─ Set compilation options

              ↓

STEP 3: Configure TLM Interfaces
├─ Add TLM 2.0 initiator/target sockets
├─ Map memory regions
├─ Configure address decoders
└─ Set timing annotations

              ↓

STEP 4: Package as Synopsys Library
├─ Create component library
├─ Add metadata and documentation
├─ Set block properties
└─ Export library package

              ↓

STEP 5: Integrate into VDK
├─ Open/Create VDK project
├─ Add component from library
├─ Connect to system bus
└─ Configure system parameters

              ↓

STEP 6: Build Virtual Prototype
├─ Generate SystemC code
├─ Compile with VDK build system
├─ Link with SystemC/TLM libraries
└─ Create simulation executable

              ↓

STEP 7: Simulate and Debug
├─ Run simulation with VP Explorer
├─ Load software on virtual CPUs
├─ Access IP through memory map
└─ Debug with SystemC Shell

```

---

## Guide Structure

This integration guide is organized into the following documents:

### Core Guides

| File | Description | When to Use |
|------|-------------|-------------|
| `01_PREPARE_SYSTEMC_IP.md` | Prepare your SystemC IP for integration | Start here with your IP |
| `02_PLATFORM_CREATOR_IMPORT.md` | Import IP into Platform Creator | After IP is TLM-ready |
| `03_TLM_INTERFACE_SETUP.md` | Configure TLM sockets and interfaces | For memory-mapped devices |
| `04_CREATE_SYNOPSYS_LIBRARY.md` | Package IP as reusable library | For sharing/reuse |
| `05_VDK_INTEGRATION.md` | Integrate into VDK Virtual Prototype | Build complete system |
| `06_BUILD_AND_SIMULATE.md` | Build, run, and debug | Test your integration |

### Advanced Guides

| File | Description |
|------|-------------|
| `07_TEMPORAL_DECOUPLING.md` | Performance optimization |
| `08_DEBUG_API_INTEGRATION.md` | GDB and debugger support |
| `09_REGISTER_INTROSPECTION.md` | Runtime register access |
| `10_DMI_SUPPORT.md` | Direct Memory Interface |
| `11_TROUBLESHOOTING.md` | Common issues and solutions |

### Reference Materials

| File | Description |
|------|-------------|
| `EXAMPLES_TLB.md` | TLB component examples |
| `API_REFERENCE.md` | Key API functions |
| `BEST_PRACTICES.md` | Design patterns and tips |

---

## Quick Start

### For Experienced Users

If you're familiar with Virtualizer, follow this quick path:

**1. Ensure IP has TLM 2.0 target socket:**
```cpp
tlm_utils::simple_target_socket<YourIP> target_socket;
```

**2. Import to Platform Creator:**
```bash
source /localdev/pdroy/keraunos_pcie_workspace/setup_virtualizer.sh
pct.exe
# File → New Component → Import SystemC
```

**3. Add to VDK:**
```bash
# In VDK Creator:
# Add Component → Your Library → Your IP
# Connect to system bus
# Build and Run
```

### For First-Time Users

**Start with these steps:**

1. **Read:** `01_PREPARE_SYSTEMC_IP.md`
2. **Follow:** Example in `EXAMPLES_TLB.md`
3. **Practice:** Build the example first
4. **Adapt:** Apply to your own IP

---

## TLB Integration Example

As a practical example, let's consider integrating an **Inbound TLB** and **Outbound TLB** into the VDK:

### TLB Component Overview

```
┌──────────────────────────────────────────┐
│         System Architecture              │
├──────────────────────────────────────────┤
│                                          │
│  ┌────────┐      ┌─────────────┐       │
│  │  CPU   │─────►│ Outbound TLB│──►    │
│  └────────┘      │ (VA→PA)     │       │
│                  └─────────────┘       │
│                                          │
│  ┌────────┐      ┌─────────────┐       │
│  │ Device │◄─────│  Inbound TLB│◄──    │
│  └────────┘      │ (IOVA→PA)   │       │
│                  └─────────────┘       │
└──────────────────────────────────────────┘
```

**Outbound TLB:**
- Translates CPU virtual addresses to physical addresses
- Used by processor-initiated transactions
- TLM initiator socket (to memory)
- TLM target socket (from CPU)

**Inbound TLB:**
- Translates device I/O virtual addresses to physical
- Used by device-initiated DMA transactions  
- TLM initiator socket (to memory)
- TLM target socket (from devices)

Both TLBs will be integrated following the same workflow.

---

## Integration Time Estimates

| Task | Basic | Full | Advanced |
|------|-------|------|----------|
| Prepare IP | 2 hours | 1 day | 2 days |
| Import to PCT | 1 hour | 4 hours | 1 day |
| TLM Setup | 2 hours | 1 day | 2 days |
| Library Package | 1 hour | 4 hours | 1 day |
| VDK Integration | 2 hours | 1 day | 2 days |
| Testing | 4 hours | 2 days | 1 week |
| **TOTAL** | **1-2 days** | **1 week** | **2-3 weeks** |

---

## Support and Resources

### Official Documentation

Located in: `/tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/Documentation/docs/vphtml/`

**Key Documents:**
- `PAVP_CoreIntegration.html` - SystemC integration concepts
- `VP_VDKGenGuide.html` - VDK generation guide
- `PAVP_SystemCShell.html` - SystemC Shell reference
- `PAVP_VPExplorer.html` - VP Explorer user guide

### Online Resources

- **Synopsys SolvNetPlus**: Support portal
- **VP Support**: vp_support@synopsys.com
- **Training**: Available through Synopsys

### Synopsys Tools Documentation

Access HTML docs in browser:
```bash
firefox /tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/Documentation/docs/vphtml/index.html &
```

---

## Getting Help

### Common Issues

See `11_TROUBLESHOOTING.md` for:
- Compilation errors
- Linking problems
- Simulation crashes
- Memory map conflicts
- Timing issues

### Debug Mode

Enable detailed logging:
```bash
export VP_DEBUG=1
export SYSTEMC_VERBOSE=1
```

### Support Files Needed

When requesting support, provide:
1. SystemC source files
2. Platform Creator project
3. Build logs
4. Error messages
5. VDK configuration

---

## Next Steps

**Ready to start?**

→ Begin with **`01_PREPARE_SYSTEMC_IP.md`**

**Need an example first?**

→ Check **`EXAMPLES_TLB.md`** for a complete TLB integration walkthrough

**Have questions?**

→ See **`11_TROUBLESHOOTING.md`** or contact Synopsys support

---

## Document Conventions

Throughout this guide:

- **`Command`** - Commands to type
- **Bold** - Important terms/concepts
- _Italic_ - File names, paths
- `Code` - Code snippets
- ⚠️ **Warning** - Important notes
- ✓ **Tip** - Helpful hints
- 📝 **Note** - Additional information

---

**Copyright © 2025 - Internal Use Only**  
**Based on Synopsys Virtualizer V-2024.03 Documentation**

