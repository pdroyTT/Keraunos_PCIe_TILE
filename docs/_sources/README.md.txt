# Keraunos PCIE Tile Documentation

This directory contains comprehensive documentation for the SystemC/TLM2.0 implementation of the Keraunos PCIE Tile components.

## Documents

### 1. Design Document
**File:** `Keraunos_PCIE_Tile_SystemC_Design_Document.md`

Comprehensive design document covering:
- System architecture and component hierarchy
- Detailed component design (TLBs and MSI Relay Unit)
- Interface specifications (TLM2.0, APB, AXI4)
- Implementation details and algorithms
- Modeling approach and design decisions
- Performance considerations

**Use this document for:**
- Understanding the overall architecture
- Learning how components are designed
- Reference for implementation details
- Integration guidance

### 2. Testplan Document
**File:** `Keraunos_PCIE_Tile_Testplan.md`

Comprehensive testplan covering:
- Test environment and infrastructure
- Test strategy and methodology
- Detailed test cases for all components
- Coverage goals and metrics
- Regression testing procedures

**Use this document for:**
- Planning verification activities
- Understanding test requirements
- Executing test cases
- Tracking coverage goals

## Document Structure

```{mermaid}
graph TD
    Doc["doc/"]
    Doc --> README["README.md<br/>(This file)"]
    Doc --> Design["Keraunos_PCIE_Tile_SystemC_Design_Document.md<br/>(Design document)"]
    Doc --> Testplan["Keraunos_PCIE_Tile_Testplan.md<br/>(Testplan document)"]
    
    style Doc fill:#e1f5ff
    style README fill:#fff4e1
    style Design fill:#e8f5e9
    style Testplan fill:#fce4ec
```

## Quick Reference

### Component Overview

**Inbound TLBs:**
- TLBSysIn0: 64 entries, 16KB pages, system management
- TLBAppIn0: 64 entries, 16MB pages, BAR0/1 (4 instances)
- TLBAppIn1: 64 entries, 8GB pages, BAR4/5

**Outbound TLBs:**
- TLBSysOut0: 16 entries, 64KB pages, DBI access
- TLBAppOut0: 16 entries, 16TB pages, high addresses (>=256TB)
- TLBAppOut1: 16 entries, 64KB pages, DBI access (<256TB)

**MSI Relay Unit:**
- 16 MSI-X vectors
- Pending Bit Array (PBA)
- MSI-X table (16 entries)
- MSI thrower process

### Key Specifications

- **TLB Entry Size:** 64 bytes
- **MSI-X Vectors:** 16 (configurable)
- **CSR Space:** 16KB for MSI Relay Unit
- **Address Width:** 64-bit for PCIe, 52-bit for NOC/SMN
- **Data Width:** 256-bit (NOC), 64-bit (SMN), 32-bit (APB)

## Related Files

- **Source Code:** `../include/` and `../src/`
- **Main README:** `../README.md`
- **Specification:** Keraunos PCIE Tile Specification v0.7.023

## Document Maintenance

- **Version Control:** Documents are version controlled
- **Updates:** Update version number and date when making changes
- **Review:** Documents should be reviewed with each major release

---

For questions or updates, please contact the SystemC Modeling Team.

