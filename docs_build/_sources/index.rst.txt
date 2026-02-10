Keraunos PCIe Tile Documentation
=================================

Welcome to the Keraunos PCIe Tile SystemC/TLM2.0 implementation documentation.

.. toctree::
   :maxdepth: 3
   :caption: Contents:

   Keraunos_PCIe_Tile_HLD.md
   Keraunos_PCIE_Tile_SystemC_Design_Document.md
   Keraunos_PCIE_Tile_Testplan.md
   ENABLE_GATING_DESIGN.md

Overview
--------

This documentation describes the SystemC/TLM2.0 implementation of the Keraunos PCIe Tile,
featuring:

* ✅ **E126 Error Eliminated** - FastBuild compatible architecture
* ✅ **100% Test Pass Rate** - 81/81 tests passing (41 E2E + 40 directed)
* ✅ **Zero Memory Leaks** - Smart pointer (RAII) based design
* ✅ **Modern C++17** - Best practices throughout
* ✅ **SCML2 Memory** - Proper persistent storage
* ✅ **Temporal Decoupling** - Full TLM-2.0 LT support

Key Features
------------

The refactored architecture (v2.0) introduces:

1. **Function Callback-Based Communication** - Eliminates internal socket bindings
2. **Smart Pointer Memory Management** - Zero memory leaks guaranteed
3. **SCML2 Integration** - Proper configuration persistence
4. **Comprehensive Testing** - 81 test cases with cross-socket data verification
5. **Sparse Backing Memory** - Custom ``sparse_backing_memory`` with 256TB address range
6. **Initiator Socket Architecture** - DUT output ports use ``simple_initiator_socket`` for outbound traffic

Components
----------

The tile includes:

* Translation Lookaside Buffers (TLBs) - 6 inbound, 3 outbound
* MSI Relay Unit - Interrupt management
* Intra-Tile Fabric Switches - NOC-PCIE, NOC-IO, SMN-IO
* System Information Interface (SII)
* Configuration Register Block
* Clock & Reset Control
* PLL/CGM (Clock Generation Module)
* PCIe PHY Model

Documentation Sections
----------------------

* **Section 1**: Introduction and Refactored Architecture Overview
* **Section 2**: System Overview
* **Section 3**: Architecture with Mermaid Diagrams
* **Section 4**: Detailed Component Design
* **Section 5**: Interface Specifications
* **Section 6**: Implementation Details
* **Section 7**: Modeling Approach
* **Section 8**: Performance Considerations
* **Section 9**: Detailed Implementation Architecture
* **Section 10**: Implementation Guide
* **Appendices**: Component Summary, Address Maps, Acronyms

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
