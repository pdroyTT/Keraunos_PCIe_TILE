/**
 * @file fastrack_logging_example.cpp
 * @brief Example demonstrating FastTrack logging usage in Keraunos PCIe Tile
 * 
 * This example shows how to enable/disable FastTrack debug logging
 * and demonstrates the different types of log messages generated.
 */

#include "keraunos_pcie_tile.h"
#include <systemc>
#include <tlm>

using namespace keraunos::pcie;

/**
 * @brief Simple testbench demonstrating FastTrack logging control
 */
int sc_main(int argc, char* argv[]) {
    // Create PCIe tile instance
    KeraunosPcieTile pcie_tile("pcie_tile");
    
    // === OPTION 1: Enable debug logging (DEFAULT) ===
    // Debug logging is enabled by default, but you can explicitly set it:
    pcie_tile.set_debug_logging_enabled(true);
    std::cout << "Debug logging enabled: " << pcie_tile.is_debug_logging_enabled() << std::endl;
    
    // === OPTION 2: Disable debug logging ===
    // To disable all debug messages (reduces simulation output):
    // pcie_tile.set_debug_logging_enabled(false);
    
    // === OPTION 3: Toggle during simulation ===
    // You can enable/disable at any point during simulation:
    // pcie_tile.set_debug_logging_enabled(false);  // Disable
    // ... run some simulation ...
    // pcie_tile.set_debug_logging_enabled(true);   // Re-enable
    
    // Create input signals
    sc_core::sc_signal<bool> cold_reset_n;
    sc_core::sc_signal<bool> warm_reset_n;
    sc_core::sc_signal<bool> isolate_req;
    sc_core::sc_signal<bool> pcie_core_clk;
    sc_core::sc_signal<bool> axi_clk;
    
    // Connect signals
    pcie_tile.cold_reset_n(cold_reset_n);
    pcie_tile.warm_reset_n(warm_reset_n);
    pcie_tile.isolate_req(isolate_req);
    pcie_tile.pcie_core_clk(pcie_core_clk);
    pcie_tile.axi_clk(axi_clk);
    
    // Initialize signals
    cold_reset_n.write(true);
    warm_reset_n.write(true);
    isolate_req.write(false);
    pcie_core_clk.write(true);
    axi_clk.write(true);
    
    // You'll see FastTrack initialization messages like:
    // "0 ps HARDWARE.pcie_tile Info[Functional Log] Initializing Keraunos PCIe Tile: pcie_tile"
    // "0 ps HARDWARE.pcie_tile Info[Functional Log] Creating internal components..."
    
    // Set Bus Master Enable (will generate log if debug enabled)
    pcie_tile.set_bus_master_enable(true);
    
    // Run simulation
    sc_core::sc_start(1, sc_core::SC_US);
    
    std::cout << "\n=== Simulation Complete ===" << std::endl;
    std::cout << "Check simulation output for FastTrack log messages" << std::endl;
    
    return 0;
}

/**
 * @brief Example showing FastTrack message types you'll see
 * 
 * When debug_logging_enabled = true, you'll see messages like:
 * 
 * FUNCTIONAL_LOG Messages:
 *   - Component initialization
 *   - High-level operational state changes
 *   - NOC_N/SMN_N transaction entry/exit
 *   - PCIe Controller transaction flow
 *   - Routing decisions
 *   - Config register reads/writes
 *   - SYSTEM_READY register access
 *   - PCIE_ENABLE register access
 * 
 * FUNCTIONAL_LOG_VERBOSE Messages:
 *   - Detailed BME qualification checks
 *   - Verbose transaction details
 * 
 * CONFIGURATION_INFO Messages:
 *   - System ready state changes
 *   - PCIe enable changes (inbound/outbound)
 *   - BME state changes
 *   - Controller mode changes (EP/RP)
 *   - Reset state transitions
 * 
 * GENERIC_WARNING Messages:
 *   - Transactions blocked (isolation, disabled enables)
 *   - System not ready for bypass paths
 *   - Uninitialized components
 * 
 * FUNCTIONAL_ERROR Messages:
 *   - Invalid command types
 *   - Routing errors
 * 
 * ACCESS_UNMAPPED_ADDRESS Errors:
 *   - Reads/writes to unmapped config addresses
 * 
 * ACCESS_PERMISSION_CHECK_FAIL Errors:
 *   - BME permission check failures
 */

/**
 * @brief Filtering FastTrack Messages
 * 
 * Method 1: Runtime Control in Code
 *   tile.set_debug_logging_enabled(false);  // Disable all debug logs
 * 
 * Method 2: FastTrack Category Filtering (Platform Architect)
 *   - Use GUI to select which categories to display
 *   - Filter by severity: Info, Warning, Error
 *   - Filter by module hierarchy
 * 
 * Method 3: Command-line (when launching simulation)
 *   - Check your simulation environment documentation
 *   - Typically: --fastrack-filter or environment variables
 * 
 * Method 4: SCML Properties (Advanced)
 *   - Set properties in configuration files
 *   - Use scml_property<bool> for granular control
 */
