Keraunos PCIe Tile Documentation
=====================================

**SystemC/TLM2.0 Implementation - Version 1.0**

Welcome to the comprehensive documentation for the Keraunos PCIe Tile, a high-performance PCIe interconnect bridge implemented in SystemC/TLM2.0.

Overview
--------

The Keraunos PCIe Tile provides a sophisticated bridge between Network-on-Chip (NOC), System Management Network (SMN), and PCIe controller interfaces. This documentation covers:

* **High-Level Design** - Architecture, block diagrams, and system integration
* **SystemC Implementation** - Detailed low-level design and implementation specifics
* **Test Plan & Verification** - Comprehensive test coverage and verification results

Key Features
------------

* **3 Fabric Switches** - NOC-PCIE, NOC-IO, SMN-IO routing infrastructure
* **8 Translation Lookaside Buffers (TLBs)** - 5 inbound + 3 outbound address translation
* **MSI-X Relay Unit** - Message Signaled Interrupt handling with 16 vectors
* **System Information Interface (SII)** - Configuration tracking and interrupt generation
* **Clock & Reset Control** - Comprehensive power management
* **76 Test Cases** - 100% functional coverage with 251 assertions

Documentation Structure
-----------------------

.. toctree::
   :maxdepth: 2
   :caption: Design Documentation
   
   hld
   systemc_design

.. toctree::
   :maxdepth: 2
   :caption: Verification Documentation
   
   testplan

Quick Links
-----------

* :ref:`genindex`
* :ref:`search`

Document Information
--------------------

:Version: 1.0
:Date: February 2026
:Status: Released
:Test Coverage: 76 tests, 251 assertions, 0 failures

