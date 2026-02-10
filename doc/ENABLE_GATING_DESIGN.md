# Enable Gating Feature - Design Documentation

**Version:** 1.0  
**Date:** February 2026  
**Feature:** Independent Inbound/Outbound Traffic Control

## Overview

The enable gating feature provides independent control of PCIe inbound and outbound application traffic through two dedicated enable bits.

## Register Interface

**PCIE_ENABLE Register Address:** `0x18000000 + 0x0FFF8`

- Bit 0: `pcie_outbound_app_enable` (controls NOC→PCIe)
- Bit 16: `pcie_inbound_app_enable` (controls PCIe→NOC)

## Implementation

### Signal Propagation

```
APB Write → ConfigRegBlock::process_write() 
    → change_callback_() 
    → KeraunosPcieTile::update_config_dependent_modules()
    → NocPcieSwitch::set_pcie_*_enable()
```

### Key Features

1. **Callback Mechanism:** ConfigRegBlock notifies tile via `std::function` callback
2. **Status Register Bypass:** Status register checked BEFORE enable bits
3. **Independent Control:** Each direction controlled separately
4. **Recovery:** Cold reset restores defaults

## Test Coverage

- TC_NEGATIVE_ENABLE_001: Inbound disabled blocks PCIe→NOC
- TC_NEGATIVE_ENABLE_002: Outbound disabled blocks NOC→PCIe  
- TC_NEGATIVE_ENABLE_003: Both disabled - complete isolation
- TC_NEGATIVE_ENABLE_004: Both enabled - positive control

**Status:** 4 tests, 44 checks, 100% pass rate
