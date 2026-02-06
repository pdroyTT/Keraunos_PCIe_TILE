# Keraunos PCIe Tile Documentation Guide

**Version:** 1.0  
**Build Date:** February 2026  
**Build Status:** ✅ Success  

---

## Overview

This directory contains the complete Sphinx-generated documentation for the Keraunos PCIe Tile SystemC/TLM2.0 implementation. The documentation includes interactive Mermaid diagrams for enhanced visualization.

## Documentation Structure

### Main Documents

1. **High-Level Design (HLD)**
   - File: `hld.md`
   - Content: Architecture overview, block diagrams, module descriptions, interface specs
   - Diagrams: 10+ Mermaid diagrams (architecture, data flows, state machines)

2. **SystemC Implementation Design**
   - File: `systemc_design.md`
   - Content: Detailed low-level SystemC/TLM2.0 implementation
   - Includes: Class diagrams, API specifications, implementation details

3. **Test Plan and Verification**
   - File: `testplan.md`
   - Content: 76 test cases, test traceability matrix, coverage analysis
   - Results: 251 assertions, 0 failures, 100% functional coverage

### Generated HTML

The complete HTML documentation is available in:
```
_build/html/
```

## Viewing the Documentation

### Option 1: Local Web Browser

Open the main page in your web browser:

```bash
firefox _build/html/index.html
# or
google-chrome _build/html/index.html
# or
xdg-open _build/html/index.html
```

### Option 2: Python HTTP Server

Serve the documentation on a local web server:

```bash
cd _build/html
python3 -m http.server 8000
```

Then open: `http://localhost:8000` in your browser

### Option 3: Direct File Access

Navigate to:
```
file:///localdev/pdroy/keraunos_pcie_workspace/docs/_build/html/index.html
```

## Key Features

### Interactive Diagrams

- **Mermaid Diagrams:** 15+ interactive diagrams including:
  - Top-level architecture
  - Detailed block diagrams
  - Data flow sequences
  - State machines
  - Clock/reset hierarchies
  - Test coverage breakdown

### Comprehensive Coverage

- **3 Main Documents:** HLD, SystemC Design, Test Plan
- **76 Test Cases:** Fully documented with traceability
- **Address Maps:** Complete routing tables and configuration space
- **Known Limitations:** 8 documented findings with workarounds

### Navigation

- **Sidebar Navigation:** Quick access to all sections
- **Search Functionality:** Full-text search across all documents
- **Cross-References:** Links between related sections
- **Table of Contents:** Expandable TOC for each document

## Building the Documentation

If you need to rebuild the documentation:

```bash
cd /localdev/pdroy/keraunos_pcie_workspace/docs
sphinx-build -b html . _build/html
```

### Prerequisites

- Sphinx 5.3.0 (installed)
- myst-parser (installed)
- sphinxcontrib-mermaid 0.7.1 (installed)

## Document Organization

```
docs/
├── conf.py                 # Sphinx configuration
├── index.rst               # Main landing page
├── hld.md                  # High-Level Design
├── systemc_design.md       # SystemC Implementation
├── testplan.md             # Test Plan & Verification
├── _static/
│   └── custom.css          # Custom styling
└── _build/
    └── html/               # Generated HTML
        ├── index.html      # Main page
        ├── hld.html        # HLD document
        ├── systemc_design.html
        ├── testplan.html
        └── ...
```

## Excluded Content

The following intermediate/debug documents are excluded from the main documentation:

- Build status reports
- Installation procedures
- Debug logs
- Memory leak fixes
- Intermediate implementation updates
- Version compatibility notes
- SCML compliance checks (development-phase docs)

**Focus:** Final design and test documentation only

## Document Information

| Property | Value |
|----------|-------|
| Project | Keraunos PCIe Tile |
| Version | 1.0 |
| Implementation | SystemC/TLM2.0 |
| Test Coverage | 76 tests, 251 assertions, 0 failures |
| Documentation Format | Sphinx HTML with Mermaid diagrams |
| Page Count | ~1300 lines HLD + ~5100 lines SystemC + ~2700 lines Test Plan |

## Quick Links

### Main Documentation Pages

- **Home:** `_build/html/index.html`
- **High-Level Design:** `_build/html/hld.html`
- **SystemC Design:** `_build/html/systemc_design.html`
- **Test Plan:** `_build/html/testplan.html`

### Key Sections

#### High-Level Design
- Section 2: Architecture Overview with diagrams
- Section 3: Detailed Block Diagram
- Section 4: Data Flow Paths (5 sequence diagrams)
- Section 5: Module Descriptions
- Section 6: Address Map
- Section 7: Clock and Reset Strategy
- Section 10: Test Coverage Summary

#### SystemC Design
- Detailed class implementations
- TLM socket configurations
- SCML2 memory models
- Callback architectures

#### Test Plan
- Section 1.5: Test Traceability Matrix (76 tests)
- End-to-End test descriptions
- Directed test specifications
- Known findings and limitations

## Styling and Customization

The documentation uses custom CSS (`_static/custom.css`) for:

- Responsive diagram sizing
- Enhanced table styling
- Improved code block formatting
- Color-coded diagrams
- Print-friendly layouts

## Support and Updates

For updates or questions about the documentation:

1. Check `conf.py` for Sphinx configuration
2. Review `index.rst` for document structure
3. Update individual `.md` files for content changes
4. Rebuild with `sphinx-build -b html . _build/html`

---

**Documentation Generated:** February 5, 2026  
**Build Tool:** Sphinx 5.3.0 with Mermaid 10.6.1  
**Status:** ✅ Production Ready
