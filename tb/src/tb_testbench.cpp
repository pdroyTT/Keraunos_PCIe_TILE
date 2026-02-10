#include "../include/tb_testbench.h"
#include "../include/tb_testcases.h"
#include <iostream>
#include <systemc>
#include <sc_dt.h>

namespace keraunos {
namespace pcie {
namespace tb {

Testbench::Testbench(sc_core::sc_module_name name)
    : sc_module(name)
    , tlb_sys_in0_(nullptr)
    , tlb_app_in0_(nullptr)
    , tlb_app_in1_(nullptr)
    , tlb_sys_out0_(nullptr)
    , tlb_app_out0_(nullptr)
    , tlb_app_out1_(nullptr)
    , msi_relay_(nullptr)
    , noc_pcie_switch_(nullptr)
    , config_reg_(nullptr)
    , sii_block_(nullptr)
    , clock_reset_ctrl_(nullptr)
    , tlb_stimulus_(nullptr)
    , apb_stimulus_(nullptr)
    , monitor_(nullptr)
    , scoreboard_(nullptr)
    , ref_tlb_sys_in0_(nullptr)
    , ref_tlb_app_in0_(nullptr)
    , ref_tlb_app_out0_(nullptr)
    , system_ready_("system_ready")
    , msix_enable_("msix_enable")
    , msix_mask_("msix_mask")
    , setip_("setip")
    , cold_reset_n_("cold_reset_n")
    , warm_reset_n_("warm_reset_n")
    , isolate_req_("isolate_req")
    , pcie_clock_("pcie_clock")
    , ref_clock_("ref_clock")
{
    // Initialize signals
    system_ready_.write(false);
    msix_enable_.write(false);
    msix_mask_.write(false);
    setip_.write(0);
    cold_reset_n_.write(true);
    warm_reset_n_.write(true);
    isolate_req_.write(false);
    pcie_clock_.write(false);
    ref_clock_.write(false);
    
    // Create DUT instances
    tlb_sys_in0_ = new TLBSysIn0("tlb_sys_in0");
    tlb_app_in0_ = new TLBAppIn0("tlb_app_in0");
    tlb_app_in1_ = new TLBAppIn1("tlb_app_in1");
    tlb_sys_out0_ = new TLBSysOut0("tlb_sys_out0");
    tlb_app_out0_ = new TLBAppOut0("tlb_app_out0");
    tlb_app_out1_ = new TLBAppOut1("tlb_app_out1");
    msi_relay_ = new MsiRelayUnit("msi_relay");
    noc_pcie_switch_ = new NocPcieSwitch("noc_pcie_switch");
    config_reg_ = new ConfigRegBlock("config_reg");
    sii_block_ = new SiiBlock("sii_block");
    clock_reset_ctrl_ = new ClockResetControl("clock_reset_ctrl");
    
    // Create test infrastructure
    tlb_stimulus_ = new TlbStimulusGenerator("tlb_stimulus");
    apb_stimulus_ = new ApbStimulusGenerator("apb_stimulus");
    monitor_ = new TransactionMonitor("monitor");
    scoreboard_ = new Scoreboard();
    
    // Create reference models
    ref_tlb_sys_in0_ = new TlbSysIn0ReferenceModel();
    ref_tlb_app_in0_ = new TlbAppIn0ReferenceModel();
    ref_tlb_app_out0_ = new TlbAppOut0ReferenceModel();
    
    // Connect components
    connect_components();
}

Testbench::~Testbench() {
    delete tlb_sys_in0_;
    delete tlb_app_in0_;
    delete tlb_app_in1_;
    delete tlb_sys_out0_;
    delete tlb_app_out0_;
    delete tlb_app_out1_;
    delete msi_relay_;
    delete noc_pcie_switch_;
    delete config_reg_;
    delete sii_block_;
    delete clock_reset_ctrl_;
    delete tlb_stimulus_;
    delete apb_stimulus_;
    delete monitor_;
    delete scoreboard_;
    delete ref_tlb_sys_in0_;
    delete ref_tlb_app_in0_;
    delete ref_tlb_app_out0_;
}

void Testbench::connect_components() {
    // Connect TLB Sys In0 signals
    tlb_sys_in0_->system_ready(system_ready_);
    
    // Connect MSI Relay signals
    // MSI Relay doesn't have system_ready input - it's handled by ConfigRegBlock
    // msi_relay_->system_ready(system_ready_);
    msi_relay_->msix_enable(msix_enable_);
    msi_relay_->msix_mask(msix_mask_);
    msi_relay_->setip(setip_);
    
    // Connect Switch signals
    noc_pcie_switch_->isolate_req(isolate_req_);
    noc_pcie_switch_->system_ready(system_ready_);
    
    // Connect Clock/Reset signals
    clock_reset_ctrl_->cold_reset_n(cold_reset_n_);
    clock_reset_ctrl_->warm_reset_n(warm_reset_n_);
    clock_reset_ctrl_->pcie_clock(pcie_clock_);
    clock_reset_ctrl_->ref_clock(ref_clock_);
    
    // Connect sockets (simplified - actual connections depend on test)
    // These will be connected dynamically in test cases
}

void Testbench::add_test_case(std::shared_ptr<TestCase> test) {
    test_cases_.push_back(test);
}

void Testbench::run_tests() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    unsigned int passed = 0;
    unsigned int failed = 0;
    unsigned int skipped = 0;
    
    for (auto& test : test_cases_) {
        std::cout << "\nRunning: " << test->name() << std::endl;
        
        try {
            test->setup();
            test->run();
            test->teardown();
            
            if (test->result() == TestResult::PASS) {
                passed++;
                std::cout << "PASS: " << test->name() << std::endl;
            } else if (test->result() == TestResult::FAIL) {
                failed++;
            } else {
                skipped++;
            }
        } catch (const std::exception& e) {
            failed++;
            std::cout << "EXCEPTION in " << test->name() << ": " 
                     << e.what() << std::endl;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << test_cases_.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Skipped: " << skipped << std::endl;
    std::cout << "========================================" << std::endl;
}

} // namespace tb
} // namespace pcie
} // namespace keraunos

