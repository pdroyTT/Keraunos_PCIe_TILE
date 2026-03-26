Keraunos PCIe Tile Documentation
=================================

Welcome to the Keraunos PCIe Tile SystemC/TLM2.0 implementation documentation.

.. toctree::
   :maxdepth: 3
   :caption: Architecture & Design:

   Keraunos_System_Architecture.md
   Keraunos_PCIe_Tile_HLD.md
   Keraunos_PCIE_Tile_SystemC_Design_Document.md

.. toctree::
   :maxdepth: 3
   :caption: VDK Platform & Integration:

   VDK_Platform_Linux_PCIe_Guide.md

.. toctree::
   :maxdepth: 3
   :caption: Verification:

   Keraunos_PCIE_Tile_Testplan.md
   ENABLE_GATING_DESIGN.md

Overview
--------

This documentation describes the SystemC/TLM2.0 implementation of the Keraunos PCIe Tile,
featuring:

* **Linux-Booting VDK Platform** - Full RISC-V Linux on host with PCIe endpoint enumeration
* **End-to-End PCIe Data Path** - Host to Target_Memory via RC, EP, PCIE_TILE, and noc_n_initiator
* **pcie_xfer Application** - Userspace tool for interactive BAR0 read/write operations
* **Test Suite** - 86 tests (41 E2E + 45 directed), 344 checks, including BME (Bus Master Enable) tests
* **Zero Memory Leaks** - Smart pointer (RAII) based design
* **Modern C++17** - Best practices throughout
* **SCML2 Memory** - Proper persistent storage
* **Temporal Decoupling** - Full TLM-2.0 LT support

Key Features
------------

The refactored architecture (v2.0) introduces:

1. **Dual-Chiplet VDK Platform** - Host_Chiplet (RC + Linux) and Keraunos_PCIE_Chiplet (EP + Tile + Memory)
2. **Linux PCIe Stack Integration** - snps,dw-pcie driver enumerates EP, assigns BARs
3. **Function Callback-Based Communication** - Eliminates internal socket bindings
4. **Smart Pointer Memory Management** - Zero memory leaks guaranteed
5. **SCML2 Integration** - Proper configuration persistence
6. **Comprehensive Testing** - 86 test cases (incl. 5 BME tests) with cross-socket data verification
7. **Sparse Backing Memory** - Custom ``sparse_backing_memory`` with 256TB address range
8. **Initiator Socket Architecture** - DUT output ports use ``simple_initiator_socket`` for outbound traffic

Platform Architecture
---------------------

The validated VDK platform demonstrates the complete data path::

    Host CPU (Linux) → RC AXI_Slave → iATU → PCIe TLP
      → EP PCIMem_Slave → EP BusMaster
        → PCIE_TILE.pcie_controller_target
          → noc_n_initiator → Target_Memory (16 MB)

See :doc:`VDK_Platform_Linux_PCIe_Guide` for full platform details.

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

**System Architecture** (Keraunos_System_Architecture.md):

* Grendel chiplet ecosystem and Keraunos-E100 role
* PCIe Tile position and interfaces
* Model integration (Host-RC-EP-Tile connections)
* VDK integration with Synopsys DesignWare PCIe
* Final VDK platform: Linux boot, PCIe enumeration, data transfer

**VDK Platform Guide** (VDK_Platform_Linux_PCIe_Guide.md):

* Dual-chiplet topology and wiring
* Memory maps (host and device)
* Linux boot flow via OpenSBI
* PCIe enumeration and BAR assignment
* pcie_xfer application usage
* VP configuration reference
* Troubleshooting guide

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
