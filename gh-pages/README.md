# Keraunos PCIe Tile Documentation

**Professional HTML Documentation with Interactive Diagrams**

This directory contains the complete Sphinx-generated HTML documentation for the Keraunos PCIe Tile SystemC/TLM2.0 implementation.

## 📚 View Online

**[View Documentation](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)**

## 📖 Documentation Contents

### Main Documents

1. **[High-Level Design (HLD)](hld.html)**
   - Architecture overview with 16 interactive Mermaid diagrams
   - Block diagrams and data flow sequences
   - Module descriptions and interface specifications
   - Complete address maps
   - Clock and reset strategy

2. **[SystemC Implementation Design](systemc_design.html)**
   - Detailed low-level SystemC/TLM2.0 design (~5100 lines)
   - Class implementations and API specifications
   - SCML2 memory models
   - Callback architectures

3. **[Test Plan & Verification](testplan.html)**
   - 76 test cases fully documented
   - Test traceability matrix
   - 251 assertions, 0 failures
   - 100% functional coverage

### Key Features

- ✅ **16 Interactive Mermaid Diagrams**
  - Sequence diagrams for data flows
  - State machines for control logic
  - Architecture and block diagrams
  - Test coverage visualizations

- ✅ **Enhanced Visualization**
  - Large diagrams (800-1200px)
  - 16-18px fonts for readability
  - Professional styling
  - Responsive design

- ✅ **Comprehensive Coverage**
  - 3 fabric switches (NOC-PCIE, NOC-IO, SMN-IO)
  - 8 TLBs (5 inbound + 3 outbound)
  - MSI-X relay with 16 vectors
  - System Information Interface (SII)
  - Complete verification results

## 🔄 Last Updated

**Date:** $(date '+%Y-%m-%d %H:%M:%S')  
**Build Tool:** Sphinx 5.3.0 with Mermaid 10.6.1  
**Status:** Production Ready ✅

## 📊 Statistics

- **Total Pages:** 4 main documents + index
- **Total Size:** $(du -sh . 2>/dev/null | cut -f1 || echo "N/A")
- **Diagrams:** 16 interactive Mermaid diagrams
- **Test Cases:** 76 (41 E2E + 35 directed)
- **Assertions:** 251 (0 failures)
- **Code Coverage:** 100% functional coverage

## 🎨 Features

### Mermaid Diagrams
- Top-level architecture
- Complete system architecture (Section 3.1 - EXTRA LARGE)
- 5 data flow sequence diagrams
- State machines (CII tracking)
- Clock and reset hierarchies
- Test distribution charts

### Professional Styling
- Responsive design
- Color-coded tables and diagrams
- Full-text search
- Clean navigation sidebar
- Print-friendly layouts

## 📝 Documentation Sources

Generated from markdown sources in:
- `/docs/hld.md` - High-Level Design
- `/docs/systemc_design.md` - SystemC Implementation
- `/docs/testplan.md` - Test Plan & Verification

## 🛠️ Build Information

- **Sphinx Version:** 5.3.0
- **Mermaid Version:** 10.6.1 (via sphinxcontrib-mermaid 0.7.1)
- **Theme:** Alabaster with custom CSS
- **Extensions:** myst_parser, sphinxcontrib.mermaid

## 📂 Repository

Source code and documentation: [Keraunos_PCIe_TILE](https://github.com/pdroyTT/Keraunos_PCIe_TILE)

---

*Auto-generated from markdown sources using Sphinx*
