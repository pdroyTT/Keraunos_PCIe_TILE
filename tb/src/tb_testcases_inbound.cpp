#include "../include/tb_testcases.h"
#include "../include/tb_testbench.h"
#include "../include/tb_common.h"
#include "../include/keraunos_pcie_tlb_common.h"
#include <systemc>
#include <iostream>
#include <iomanip>

namespace keraunos {
namespace pcie {
namespace tb {

//=============================================================================
// TC_INBOUND_SYS_001: Basic Address Translation
//=============================================================================

TC_Inbound_Sys_001::TC_Inbound_Sys_001(Testbench* tb)
    : TestCase("TC_INBOUND_SYS_001")
    , testbench_(tb)
{
}

void TC_Inbound_Sys_001::setup() {
    // Configure TLB entry 0: valid=1, addr=0x18000000, attr=0x123
    TlbEntry entry;
    entry.valid = true;
    entry.addr = 0x18000000ULL;
    entry.attr = 0x123;
    
    // Configure via reference model
    if (testbench_->ref_tlb_sys_in0_) {
        testbench_->ref_tlb_sys_in0_->configure_entry(0, entry);
    }
    
    // Configure DUT (simplified - would use APB in real test)
    // For now, we'll test via direct memory access if available
}

void TC_Inbound_Sys_001::run() {
    std::cout << "TC_INBOUND_SYS_001: Basic Address Translation" << std::endl;
    
    // Step 1: Configure TLB entry 0 (done in setup)
    
    // Step 2: Send AXI read transaction with address 0xE0000000 (iATU output)
    // This address maps to entry 0: index = (0xE0000000 >> 14) & 0x3F = 0
    uint64_t input_addr = 0xE0000000ULL;
    
    // Step 3: Verify translated address = 0x18000000
    uint64_t expected_addr = 0x18000000ULL;
    uint64_t actual_addr = 0;
    uint32_t axuser = 0;
    
    bool ref_result = false;
    if (testbench_->ref_tlb_sys_in0_) {
        ref_result = testbench_->ref_tlb_sys_in0_->lookup(input_addr, 
                                                          actual_addr, axuser);
    }
    
    check(ref_result, "Reference model lookup failed");
    check(actual_addr == expected_addr, 
          "Translated address mismatch");
    
    // Step 4: Verify AxUSER = expected value
    // AxUSER format: {ATTR[11:4], 2'b0, ATTR[1:0]}
    // ATTR = 0x123, so ATTR[11:4] = 0x12, ATTR[1:0] = 0x3
    // Expected AxUSER = {0x12, 2'b0, 0x3} = 0x123
    uint32_t expected_axuser = 0x123;
    check(axuser == expected_axuser, "AxUSER field mismatch");
    
    pass();
}

//=============================================================================
// TC_INBOUND_SYS_002: Invalid Entry Detection
//=============================================================================

TC_Inbound_Sys_002::TC_Inbound_Sys_002(Testbench* tb)
    : TestCase("TC_INBOUND_SYS_002")
    , testbench_(tb)
{
}

void TC_Inbound_Sys_002::setup() {
    // Configure TLB entry 5: valid=0
    TlbEntry entry;
    entry.valid = false;
    entry.addr = 0x0ULL;
    entry.attr = 0x0;
    
    if (testbench_->ref_tlb_sys_in0_) {
        testbench_->ref_tlb_sys_in0_->configure_entry(5, entry);
    }
}

void TC_Inbound_Sys_002::run() {
    std::cout << "TC_INBOUND_SYS_002: Invalid Entry Detection" << std::endl;
    
    // Send AXI read transaction with address that maps to entry 5
    // Index 5: address = (5 << 14) | offset = 0x14000
    uint64_t input_addr = 0xE0001400ULL;  // Maps to entry 5
    
    uint64_t translated_addr = 0;
    uint32_t axuser = 0;
    
    bool ref_result = false;
    if (testbench_->ref_tlb_sys_in0_) {
        ref_result = testbench_->ref_tlb_sys_in0_->lookup(input_addr,
                                                          translated_addr, axuser);
    }
    
    // Should return false (invalid entry)
    check(!ref_result, "Should detect invalid entry");
    check(translated_addr == INVALID_ADDRESS_DECERR, 
          "Should return DECERR address");
    
    pass();
}

//=============================================================================
// TC_INBOUND_SYS_003: Index Calculation
//=============================================================================

TC_Inbound_Sys_003::TC_Inbound_Sys_003(Testbench* tb)
    : TestCase("TC_INBOUND_SYS_003")
    , testbench_(tb)
{
}

void TC_Inbound_Sys_003::setup() {
    // Configure all 64 entries with unique addresses
    if (testbench_->ref_tlb_sys_in0_) {
        for (uint8_t i = 0; i < 64; i++) {
            TlbEntry entry;
            entry.valid = true;
            entry.addr = 0x10000000ULL + (i * 0x1000000ULL);
            entry.attr = i;
            testbench_->ref_tlb_sys_in0_->configure_entry(i, entry);
        }
    }
}

void TC_Inbound_Sys_003::run() {
    std::cout << "TC_INBOUND_SYS_003: Index Calculation" << std::endl;
    
    // Send transactions covering all index ranges
    bool all_passed = true;
    
    for (uint8_t i = 0; i < 64; i++) {
        // Calculate input address for entry i
        // Index = (addr >> 14) & 0x3F
        // So addr = (i << 14) | offset
        uint64_t input_addr = 0xE0000000ULL | (i << 14);
        
        uint64_t translated_addr = 0;
        uint32_t axuser = 0;
        
        bool ref_result = false;
        if (testbench_->ref_tlb_sys_in0_) {
            ref_result = testbench_->ref_tlb_sys_in0_->lookup(input_addr,
                                                             translated_addr, axuser);
        }
        
        if (!ref_result) {
            std::cout << "  Entry " << static_cast<int>(i) << " lookup failed" << std::endl;
            all_passed = false;
            continue;
        }
        
        // Verify correct entry was used
        TlbEntry entry = testbench_->ref_tlb_sys_in0_->get_entry(i);
        uint64_t expected_base = entry.addr & 0xFFFFFFFFFC000ULL;
        uint64_t actual_base = translated_addr & 0xFFFFFFFFFC000ULL;
        
        if (expected_base != actual_base) {
            std::cout << "  Entry " << static_cast<int>(i) 
                     << " translation mismatch" << std::endl;
            all_passed = false;
        }
    }
    
    check(all_passed, "Not all entries accessed correctly");
    pass();
}

//=============================================================================
// TC_INBOUND_SYS_004: Page Boundary Crossing
//=============================================================================

TC_Inbound_Sys_004::TC_Inbound_Sys_004(Testbench* tb)
    : TestCase("TC_INBOUND_SYS_004")
    , testbench_(tb)
{
}

void TC_Inbound_Sys_004::setup() {
    // Configure TLB entry 10: addr=0x10000000
    TlbEntry entry;
    entry.valid = true;
    entry.addr = 0x10000000ULL;
    entry.attr = 0x0;
    
    if (testbench_->ref_tlb_sys_in0_) {
        testbench_->ref_tlb_sys_in0_->configure_entry(10, entry);
    }
}

void TC_Inbound_Sys_004::run() {
    std::cout << "TC_INBOUND_SYS_004: Page Boundary Crossing" << std::endl;
    
    // Send transaction with address 0xE0001234 (offset=0x1234)
    // Entry 10: index = (0xE0000000 >> 14) & 0x3F = 0
    // Actually, we need address that maps to entry 10
    // Index 10: base = (10 << 14) = 0x28000
    uint64_t input_addr = 0xE0002800ULL | 0x1234;  // Entry 10, offset 0x1234
    
    uint64_t translated_addr = 0;
    uint32_t axuser = 0;
    
    bool ref_result = false;
    if (testbench_->ref_tlb_sys_in0_) {
        ref_result = testbench_->ref_tlb_sys_in0_->lookup(input_addr,
                                                         translated_addr, axuser);
    }
    
    check(ref_result, "Translation failed");
    
    // Verify translated address = 0x10001234 (preserves offset)
    uint64_t expected_addr = 0x10000000ULL | 0x1234;
    check(translated_addr == expected_addr, 
          "Page offset not preserved correctly");
    
    pass();
}

//=============================================================================
// TC_INBOUND_SYS_005: AxUSER Field Mapping
//=============================================================================

TC_Inbound_Sys_005::TC_Inbound_Sys_005(Testbench* tb)
    : TestCase("TC_INBOUND_SYS_005")
    , testbench_(tb)
{
}

void TC_Inbound_Sys_005::setup() {
    // Configure TLB entry with attr = 0xABC
    TlbEntry entry;
    entry.valid = true;
    entry.addr = 0x18000000ULL;
    entry.attr = 0xABC;
    
    if (testbench_->ref_tlb_sys_in0_) {
        testbench_->ref_tlb_sys_in0_->configure_entry(0, entry);
    }
}

void TC_Inbound_Sys_005::run() {
    std::cout << "TC_INBOUND_SYS_005: AxUSER Field Mapping" << std::endl;
    
    uint64_t input_addr = 0xE0000000ULL;
    uint64_t translated_addr = 0;
    uint32_t axuser = 0;
    
    bool ref_result = false;
    if (testbench_->ref_tlb_sys_in0_) {
        ref_result = testbench_->ref_tlb_sys_in0_->lookup(input_addr,
                                                         translated_addr, axuser);
    }
    
    check(ref_result, "Translation failed");
    
    // Verify AxUSER[11:4] = attr[11:4] = 0xAB
    uint32_t attr_high = (axuser >> 4) & 0xFF;
    check(attr_high == 0xAB, "AxUSER[11:4] mismatch");
    
    // Verify AxUSER[3:2] = 0b00
    uint32_t attr_mid = (axuser >> 2) & 0x3;
    check(attr_mid == 0x0, "AxUSER[3:2] should be 0");
    
    // Verify AxUSER[1:0] = attr[1:0] = 0x3
    uint32_t attr_low = axuser & 0x3;
    check(attr_low == 0x3, "AxUSER[1:0] mismatch");
    
    pass();
}

//=============================================================================
// TC_INBOUND_SYS_006: Concurrent Transactions
//=============================================================================

TC_Inbound_Sys_006::TC_Inbound_Sys_006(Testbench* tb)
    : TestCase("TC_INBOUND_SYS_006")
    , testbench_(tb)
{
}

void TC_Inbound_Sys_006::setup() {
    // Configure multiple entries
    if (testbench_->ref_tlb_sys_in0_) {
        for (uint8_t i = 0; i < 8; i++) {
            TlbEntry entry;
            entry.valid = true;
            entry.addr = 0x10000000ULL + (i * 0x1000000ULL);
            entry.attr = i;
            testbench_->ref_tlb_sys_in0_->configure_entry(i, entry);
        }
    }
}

void TC_Inbound_Sys_006::run() {
    std::cout << "TC_INBOUND_SYS_006: Concurrent Transactions" << std::endl;
    
    // Test multiple concurrent lookups (simulated sequentially)
    bool all_passed = true;
    
    for (uint8_t i = 0; i < 8; i++) {
        uint64_t input_addr = 0xE0000000ULL | (i << 14);
        
        uint64_t translated_addr = 0;
        uint32_t axuser = 0;
        
        bool ref_result = false;
        if (testbench_->ref_tlb_sys_in0_) {
            ref_result = testbench_->ref_tlb_sys_in0_->lookup(input_addr,
                                                             translated_addr, axuser);
        }
        
        if (!ref_result) {
            all_passed = false;
        }
    }
    
    check(all_passed, "Concurrent transaction handling failed");
    pass();
}

} // namespace tb
} // namespace pcie
} // namespace keraunos

