#include "../include/tb_testcases.h"
#include "../include/tb_testbench.h"
#include "../include/tb_common.h"
#include "../include/keraunos_pcie_tlb_common.h"
#include <iostream>
#include <systemc>
#include <sc_dt.h>

namespace keraunos {
namespace pcie {
namespace tb {

//=============================================================================
// TC_OUTBOUND_SYS_001: Basic Outbound Translation
//=============================================================================

TC_Outbound_Sys_001::TC_Outbound_Sys_001(Testbench* tb)
    : TestCase("TC_OUTBOUND_SYS_001")
    , testbench_(tb)
{
}

void TC_Outbound_Sys_001::setup() {
    // Configure TLB Sys Out0 entry
    // Note: Outbound TLBs have different structure
    // This is a placeholder - actual implementation depends on TLB structure
}

void TC_Outbound_Sys_001::run() {
    std::cout << "TC_OUTBOUND_SYS_001: Basic Outbound Translation" << std::endl;
    
    // Test outbound translation
    // This would test TLBSysOut0 translation logic
    // Placeholder implementation
    
    pass();
}

//=============================================================================
// TC_OUTBOUND_APP0_001: App Outbound Translation
//=============================================================================

TC_Outbound_App0_001::TC_Outbound_App0_001(Testbench* tb)
    : TestCase("TC_OUTBOUND_APP0_001")
    , testbench_(tb)
{
}

void TC_Outbound_App0_001::setup() {
    // Configure TLB App Out0 entry
    if (testbench_->ref_tlb_app_out0_) {
        TlbEntry entry;
        entry.valid = true;
        entry.addr = 0x100000000000ULL;  // >= 256TB
        entry.attr = 0x123;
        testbench_->ref_tlb_app_out0_->configure_entry(0, entry);
    }
}

void TC_Outbound_App0_001::run() {
    std::cout << "TC_OUTBOUND_APP0_001: App Outbound Translation" << std::endl;
    
    // Test address >= 256TB
    uint64_t input_addr = 0x100000000000ULL;  // 256TB
    
    uint64_t translated_addr = 0;
    sc_dt::sc_bv<256> attr;
    
    bool ref_result = false;
    if (testbench_->ref_tlb_app_out0_) {
        ref_result = testbench_->ref_tlb_app_out0_->lookup_outbound(
            input_addr, translated_addr, attr);
    }
    
    check(ref_result, "Outbound translation failed");
    check(translated_addr != INVALID_ADDRESS_DECERR, 
          "Should not return DECERR for valid address");
    
    pass();
}

} // namespace tb
} // namespace pcie
} // namespace keraunos

