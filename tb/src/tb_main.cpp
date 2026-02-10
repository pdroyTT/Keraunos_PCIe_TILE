#include "../include/tb_testbench.h"
#include "../include/tb_testcases.h"
#include <iostream>
#include <memory>
#include <systemc>

using namespace keraunos::pcie::tb;

int sc_main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Keraunos PCIE Tile Testbench" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Create testbench (stack allocation - no memory leak risk)
    Testbench tb("testbench");
    
    // Register test cases
    
    // Inbound TLB Tests
    tb.add_test_case(std::make_shared<TC_Inbound_Sys_001>(&tb));
    tb.add_test_case(std::make_shared<TC_Inbound_Sys_002>(&tb));
    tb.add_test_case(std::make_shared<TC_Inbound_Sys_003>(&tb));
    tb.add_test_case(std::make_shared<TC_Inbound_Sys_004>(&tb));
    tb.add_test_case(std::make_shared<TC_Inbound_Sys_005>(&tb));
    tb.add_test_case(std::make_shared<TC_Inbound_Sys_006>(&tb));
    
    // Outbound TLB Tests
    tb.add_test_case(std::make_shared<TC_Outbound_Sys_001>(&tb));
    tb.add_test_case(std::make_shared<TC_Outbound_App0_001>(&tb));
    
    // MSI Relay Tests
    tb.add_test_case(std::make_shared<TC_Msi_Relay_001>(&tb));
    tb.add_test_case(std::make_shared<TC_Msi_Relay_004>(&tb));
    
    // Switch Tests
    tb.add_test_case(std::make_shared<TC_Switch_Noc_Pcie_001>(&tb));
    tb.add_test_case(std::make_shared<TC_Switch_Noc_Pcie_002>(&tb));
    
    // Config Register Tests
    tb.add_test_case(std::make_shared<TC_Config_Reg_001>(&tb));
    tb.add_test_case(std::make_shared<TC_Config_Reg_002>(&tb));
    
    // Clock/Reset Tests
    tb.add_test_case(std::make_shared<TC_Clock_Reset_001>(&tb));
    tb.add_test_case(std::make_shared<TC_Clock_Reset_003>(&tb));
    
    // Run tests
    tb.run_tests();
    
    // No explicit cleanup needed - automatic destruction
    return 0;
}

