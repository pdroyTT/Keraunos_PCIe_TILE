# Keraunos PCIe Tile

**SystemC/TLM2.0 PCIe Interconnect Bridge**

## 📚 Documentation

**[View Online Documentation](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)**

Professional HTML documentation with interactive Mermaid diagrams

### Contents
- **High-Level Design** - 16 interactive diagrams
- **SystemC Implementation** - ~5100 lines of detailed design
- **Test Plan** - 76 tests, 251 assertions, 100% coverage

## Features

- 3 Fabric Switches (NOC-PCIE, NOC-IO, SMN-IO)
- 8 TLBs (5 inbound + 3 outbound)
- MSI-X Relay (16 vectors)
- System Information Interface (SII)
- Comprehensive test suite

## Build

```bash
cd docs
sphinx-build -b html . _build/html
```

## Publish

```bash
./publish_docs.sh
```

---
Copyright 2026 Tenstorrent Inc.
