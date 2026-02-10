#ifndef TB_TESTBENCH_H
#define TB_TESTBENCH_H

#include "tb_common.h"
#include "tb_stimulus_generator.h"
#include "tb_monitor.h"
#include "tb_tlb_reference_model.h"
#include "../include/keraunos_pcie_inbound_tlb.h"
#include "../include/keraunos_pcie_outbound_tlb.h"
#include "../include/keraunos_pcie_msi_relay.h"
#include "../include/keraunos_pcie_noc_pcie_switch.h"
#include "../include/keraunos_pcie_config_reg.h"
#include "../include/keraunos_pcie_sii.h"
#include "../include/keraunos_pcie_clock_reset.h"
// Include standard SystemC headers BEFORE SCML2 to avoid vtable mismatches
#include <systemc>
#include <sc_dt.h>
#include <tlm>
#include <scml2.h>
#include <vector>
#include <memory>
#include <sc_dt.h>

namespace keraunos {
namespace pcie {
namespace tb {

/**
 * Top-Level Testbench
 * 
 * Instantiates DUT and test infrastructure
 */
class Testbench : public sc_core::sc_module {
public:
    // DUT instances
    TLBSysIn0* tlb_sys_in0_;
    TLBAppIn0* tlb_app_in0_;
    TLBAppIn1* tlb_app_in1_;
    TLBSysOut0* tlb_sys_out0_;
    TLBAppOut0* tlb_app_out0_;
    TLBAppOut1* tlb_app_out1_;
    MsiRelayUnit* msi_relay_;
    NocPcieSwitch* noc_pcie_switch_;
    ConfigRegBlock* config_reg_;
    SiiBlock* sii_block_;
    ClockResetControl* clock_reset_ctrl_;
    
    // Test infrastructure
    TlbStimulusGenerator* tlb_stimulus_;
    ApbStimulusGenerator* apb_stimulus_;
    TransactionMonitor* monitor_;
    Scoreboard* scoreboard_;
    
    // Reference models
    TlbSysIn0ReferenceModel* ref_tlb_sys_in0_;
    TlbAppIn0ReferenceModel* ref_tlb_app_in0_;
    TlbAppOut0ReferenceModel* ref_tlb_app_out0_;
    
    // Signals
    sc_core::sc_signal<bool> system_ready_;
    sc_core::sc_signal<bool> msix_enable_;
    sc_core::sc_signal<bool> msix_mask_;
    sc_core::sc_signal<sc_dt::sc_bv<16> > setip_;
    sc_core::sc_signal<bool> cold_reset_n_;
    sc_core::sc_signal<bool> warm_reset_n_;
    sc_core::sc_signal<bool> isolate_req_;
    sc_core::sc_signal<bool> pcie_clock_;
    sc_core::sc_signal<bool> ref_clock_;
    
    SC_HAS_PROCESS(Testbench);
    
    Testbench(sc_core::sc_module_name name);
    virtual ~Testbench();
    
    // Test execution
    void run_tests();
    void add_test_case(std::shared_ptr<TestCase> test);
    
private:
    std::vector<std::shared_ptr<TestCase> > test_cases_;
    void connect_components();
};

} // namespace tb
} // namespace pcie
} // namespace keraunos

#endif // TB_TESTBENCH_H

