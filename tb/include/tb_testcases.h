#ifndef TB_TESTCASES_H
#define TB_TESTCASES_H

#include "tb_common.h"
#include "tb_tlb_reference_model.h"
#include "../include/keraunos_pcie_inbound_tlb.h"
#include "../include/keraunos_pcie_outbound_tlb.h"
#include "../include/keraunos_pcie_msi_relay.h"
#include "../include/keraunos_pcie_noc_pcie_switch.h"
#include "../include/keraunos_pcie_config_reg.h"
#include "../include/keraunos_pcie_sii.h"
#include "../include/keraunos_pcie_clock_reset.h"
#include <scml2.h>
#include <systemc>
#include <tlm>

namespace keraunos {
namespace pcie {
namespace tb {

// Forward declarations
class Testbench;

//=============================================================================
// Inbound TLB Test Cases
//=============================================================================

class TC_Inbound_Sys_001 : public TestCase {
public:
    TC_Inbound_Sys_001(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Inbound_Sys_002 : public TestCase {
public:
    TC_Inbound_Sys_002(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Inbound_Sys_003 : public TestCase {
public:
    TC_Inbound_Sys_003(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Inbound_Sys_004 : public TestCase {
public:
    TC_Inbound_Sys_004(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Inbound_Sys_005 : public TestCase {
public:
    TC_Inbound_Sys_005(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Inbound_Sys_006 : public TestCase {
public:
    TC_Inbound_Sys_006(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

//=============================================================================
// Outbound TLB Test Cases
//=============================================================================

class TC_Outbound_Sys_001 : public TestCase {
public:
    TC_Outbound_Sys_001(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Outbound_App0_001 : public TestCase {
public:
    TC_Outbound_App0_001(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

//=============================================================================
// MSI Relay Test Cases
//=============================================================================

class TC_Msi_Relay_001 : public TestCase {
public:
    TC_Msi_Relay_001(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Msi_Relay_004 : public TestCase {
public:
    TC_Msi_Relay_004(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

//=============================================================================
// Switch Test Cases
//=============================================================================

class TC_Switch_Noc_Pcie_001 : public TestCase {
public:
    TC_Switch_Noc_Pcie_001(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Switch_Noc_Pcie_002 : public TestCase {
public:
    TC_Switch_Noc_Pcie_002(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

//=============================================================================
// Config Register Test Cases
//=============================================================================

class TC_Config_Reg_001 : public TestCase {
public:
    TC_Config_Reg_001(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Config_Reg_002 : public TestCase {
public:
    TC_Config_Reg_002(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

//=============================================================================
// Clock/Reset Test Cases
//=============================================================================

class TC_Clock_Reset_001 : public TestCase {
public:
    TC_Clock_Reset_001(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

class TC_Clock_Reset_003 : public TestCase {
public:
    TC_Clock_Reset_003(Testbench* tb);
    void run() override;
    void setup() override;
    
private:
    Testbench* testbench_;
};

} // namespace tb
} // namespace pcie
} // namespace keraunos

#endif // TB_TESTCASES_H

