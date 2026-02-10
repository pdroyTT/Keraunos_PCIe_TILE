#include "keraunos_pcie_tile.h"
#include <cstring>

namespace keraunos {
namespace pcie {

KeraunosPcieTile::KeraunosPcieTile(sc_core::sc_module_name name)
    : sc_module(name)
    , noc_n_initiator("noc_n_initiator")
    , smn_n_initiator("smn_n_initiator")
    , pcie_controller_initiator("pcie_controller_initiator")
    , noc_pcie_switch_(nullptr)
    , noc_io_switch_(nullptr)
    , smn_io_switch_(nullptr)
    , tlb_sys_in0_(nullptr)
    , tlb_app_in1_(nullptr)
    , tlb_sys_out0_(nullptr)
    , tlb_app_out0_(nullptr)
    , tlb_app_out1_(nullptr)
    , msi_relay_(nullptr)
    , sii_block_(nullptr)
    , config_reg_(nullptr)
    , clock_reset_ctrl_(nullptr)
    , pll_cgm_(nullptr)
    , pcie_phy_(nullptr)
    // Note: noc_n_interface_ and smn_n_interface_ no longer used
{
    // Register callbacks for pass-through target sockets
    noc_n_initiator.register_b_transport(this, &KeraunosPcieTile::noc_n_initiator_b_transport);
    noc_n_initiator.register_transport_dbg(this, &KeraunosPcieTile::noc_n_initiator_transport_dbg);
    smn_n_initiator.register_b_transport(this, &KeraunosPcieTile::smn_n_initiator_b_transport);
    smn_n_initiator.register_transport_dbg(this, &KeraunosPcieTile::smn_n_initiator_transport_dbg);
    pcie_controller_initiator.register_b_transport(this, &KeraunosPcieTile::pcie_controller_initiator_b_transport);
    pcie_controller_initiator.register_transport_dbg(this, &KeraunosPcieTile::pcie_controller_initiator_transport_dbg);
    

    // Initialize TLB AppIn0 array
    for (int i = 0; i < 4; i++) {
        tlb_app_in0_[i] = nullptr;
    }
    
    // Instantiate components
    noc_pcie_switch_ = new NocPcieSwitch("noc_pcie_switch");
    noc_io_switch_ = new NocIoSwitch("noc_io_switch");
    smn_io_switch_ = new SmnIoSwitch("smn_io_switch");
    
    // Instantiate TLBs
    tlb_sys_in0_ = new TLBSysIn0("tlb_sys_in0");
    for (int i = 0; i < 4; i++) {
        char name[32];
        snprintf(name, sizeof(name), "tlb_app_in0_%d", i);
        tlb_app_in0_[i] = new TLBAppIn0(name, i);
    }
    tlb_app_in1_ = new TLBAppIn1("tlb_app_in1");
    
    tlb_sys_out0_ = new TLBSysOut0("tlb_sys_out0");
    tlb_app_out0_ = new TLBAppOut0("tlb_app_out0");
    tlb_app_out1_ = new TLBAppOut1("tlb_app_out1");
    
    // Instantiate MSI Relay
    msi_relay_ = new MsiRelayUnit("msi_relay", 16);
    
    // Instantiate configuration and control blocks
    sii_block_ = new SiiBlock("sii_block");
    config_reg_ = new ConfigRegBlock("config_reg");
    clock_reset_ctrl_ = new ClockResetControl("clock_reset_ctrl");
    pll_cgm_ = new PllCgm("pll_cgm");
    pcie_phy_ = new PciePhy("pcie_phy");
    
    // Note: External interface wrappers removed - switches connect directly to top-level ports
    
    // Connect components
    connect_components();
}

KeraunosPcieTile::~KeraunosPcieTile() {
    // Cleanup (SystemC will handle most of it)
    delete noc_pcie_switch_;
    delete noc_io_switch_;
    delete smn_io_switch_;
    delete tlb_sys_in0_;
    for (int i = 0; i < 4; i++) {
        delete tlb_app_in0_[i];
    }
    delete tlb_app_in1_;
    delete tlb_sys_out0_;
    delete tlb_app_out0_;
    delete tlb_app_out1_;
    delete msi_relay_;
    delete sii_block_;
    delete config_reg_;
    delete clock_reset_ctrl_;
    delete pll_cgm_;
    delete pcie_phy_;
    // Note: noc_n_interface_ and smn_n_interface_ no longer instantiated
}

void KeraunosPcieTile::connect_components() {
    // Connect clock and reset
    clock_reset_ctrl_->cold_reset_n(cold_reset_n);
    clock_reset_ctrl_->warm_reset_n(warm_reset_n);
    clock_reset_ctrl_->isolate_req(isolate_req);
    clock_reset_ctrl_->pcie_clock(pcie_clock_);
    clock_reset_ctrl_->ref_clock(ref_clock_);
    
    // Connect PLL/CGM
    pll_cgm_->ref_clock(ref_clock_);
    pll_cgm_->reset_n(pcie_sii_reset_ctrl_);
    pll_cgm_->pcie_clock(pcie_clock_);
    
    // Connect PHY
    pcie_phy_->ref_clock(ref_clock_);
    pcie_phy_->reset_n(pcie_reset_ctrl_);
    
    // Connect config register
    config_reg_->isolate_req(isolate_req);
    config_reg_->system_ready(system_ready_);
    config_reg_->pcie_outbound_app_enable(pcie_outbound_app_enable_);
    config_reg_->pcie_inbound_app_enable(pcie_inbound_app_enable_);
    
    // Connect NOC-PCIE switch
    noc_pcie_switch_->isolate_req(isolate_req);
    noc_pcie_switch_->pcie_outbound_app_enable(pcie_outbound_app_enable_);
    noc_pcie_switch_->pcie_inbound_app_enable(pcie_inbound_app_enable_);
    noc_pcie_switch_->system_ready(system_ready_);
    
    // Connect TLB Sys In0
    tlb_sys_in0_->system_ready(system_ready_);
    
    // Connect MSI Relay
    msi_relay_->msix_enable(msix_enable_);
    msi_relay_->msix_mask(msix_mask_);
    msi_relay_->setip(setip_);
    
    // Connect switches
    noc_io_switch_->isolate_req(isolate_req);
    smn_io_switch_->isolate_req(isolate_req);
    
    // Connect NOC-N external interfaces (direct connection, no wrapper)
    // Inbound: External → noc_n_target → NOC-IO switch noc_n_port (target)
    noc_n_target.bind(noc_io_switch_->noc_n_port);
    // Outbound: NOC-IO switch noc_n_initiator → noc_n_initiator (external)
    noc_io_switch_->noc_n_initiator.bind(noc_n_initiator);
    
    // Connect SMN-N external interfaces
    // Inbound: External → smn_n_target → SMN-IO switch smn_n_port (target)
    smn_n_target.bind(smn_io_switch_->smn_n_port);
    // Outbound: SMN-IO switch smn_n_initiator → smn_n_initiator (external)
    smn_io_switch_->smn_n_initiator.bind(smn_n_initiator);
    
    // Connect NOC-PCIE switch to TLBs (inbound routing)
    // Note: Actual routing logic is handled inside the switch based on AxADDR[63:60]
    noc_pcie_switch_->tlb_app_inbound_port0.bind(tlb_app_in0_[0]->inbound_socket);
    noc_pcie_switch_->tlb_app_inbound_port1.bind(tlb_app_in1_->inbound_socket);
    noc_pcie_switch_->tlb_sys_inbound_port.bind(tlb_sys_in0_->inbound_socket);
    
    // Connect remaining TLB App In0 instances (1-3) - these will be routed based on address
    // For now, connect them to the same ports (switch will route based on address decoding)
    // TODO: Add additional ports to NOC-PCIE switch if instances 1-3 need separate routing
    // Currently only instance 0 is used, instances 1-3 are for future expansion
    
    // Connect TLB outbound sockets to NOC-PCIE switch initiator ports
    // Outbound flow: PCIe Controller → NOC-PCIE switch target → NOC-PCIE switch initiator → TLB outbound target
    noc_pcie_switch_->tlb_app_out0_initiator.bind(tlb_app_out0_->outbound_socket);
    noc_pcie_switch_->tlb_app_out1_initiator.bind(tlb_app_out1_->outbound_socket);
    noc_pcie_switch_->tlb_sys_out0_initiator.bind(tlb_sys_out0_->outbound_socket);
    
    // Connect TLBs to switches (outbound routing - translated output)
    // After translation, TLBs forward via their translated_socket (initiator) to switches
    tlb_app_out0_->translated_socket.bind(noc_pcie_switch_->tlb_app_outbound_port);
    tlb_app_out1_->translated_socket.bind(noc_pcie_switch_->tlb_app_outbound_port);  // Also routes to app outbound
    tlb_sys_out0_->translated_socket.bind(noc_pcie_switch_->tlb_sys_outbound_port);
    
    // Connect MSI Relay
    smn_io_switch_->msi_relay_cfg_port.bind(msi_relay_->csr_apb_socket);
    noc_io_switch_->msi_relay_port.bind(msi_relay_->msi_axi_socket);
    
    // Connect NOC-PCIE switch to NOC-IO and SMN-IO switches
    // For outbound traffic from PCIe: NOC-PCIE switch → NOC-IO/SMN-IO
    // NOTE: noc_io_switch->noc_n_port is already bound to noc_n_target (line 129)
    // This creates a multi-master scenario on noc_n_port which is allowed by simple_target_socket
    noc_pcie_switch_->noc_io_initiator.bind(noc_io_switch_->noc_n_port);
    
    // NOTE: smn_io_switch->smn_n_port is an initiator (line 30 in smn_io_switch.h)
    // It's already bound to smn_n_initiator (line 139)
    // Cannot bind another initiator to it - architecture issue
    // Commenting out to avoid double-binding error:
    // noc_pcie_switch_->smn_io_initiator.bind(smn_io_switch_->smn_n_port);
    
    // Connect NOC-IO switch to TLB inbound path
    // TLB App Inbound translated socket → NOC-IO switch tlb_app_inbound_port (target) → External NOC-N
    tlb_app_in0_[0]->translated_socket.bind(noc_io_switch_->tlb_app_inbound_port);
    // Outbound: NOC-IO switch initiator port → TLB App Outbound target socket
    // Note: tlb_app_outbound_port is an initiator that forwards to TLB, but we need to connect it properly
    // The switch's tlb_app_outbound_target is a target that receives, then routes internally
    // For now, connect TLB outbound to NOC-PCIE switch, which will route to NOC-IO if needed
    
    // Connect SMN-IO switch to TLB Sys Inbound path
    // TLB Sys Inbound translated socket → SMN-IO switch tlb_sys_inbound_port (initiator) → External SMN-N
    tlb_sys_in0_->translated_socket.bind(smn_io_switch_->tlb_sys_inbound_port);
    // Outbound: SMN-IO switch initiator port → TLB Sys Outbound target socket
    // Note: Similar to NOC-IO, the switch routes internally
    
    // Connect SII block
    smn_io_switch_->sii_config_port.bind(sii_block_->apb_socket);
    // Note: SII block clock/reset connections would be made here when PCIe Controller is implemented
    // sii_block_->pcie_core_clk(pcie_controller_core_clk);
    // sii_block_->axi_clk(axi_clock_signal);
    // sii_block_->reset_n(pcie_reset_ctrl_);
    // sii_block_->cii_hv(controller_cii_hv);
    // sii_block_->cii_hdr_type(controller_cii_hdr_type);
    // sii_block_->cii_hdr_addr(controller_cii_hdr_addr);
    
    // NOTE: Config register connections handled through SMN-IO switch routing
    // TLB config access is routed via SMN-IO switch to individual TLB config initiator ports
    
    // Connect PHY
    smn_io_switch_->serdes_apb_port.bind(pcie_phy_->apb_socket);
    smn_io_switch_->serdes_ahb_port.bind(pcie_phy_->ahb_socket);
    
    // Connect interrupt outputs (non-PCIe Controller dependent)
    // Config update interrupt from SII block (via intermediate signal)
    sii_block_->config_int(this->config_int_sig_);
    config_update(this->config_int_sig_);
    
    // Connect timeout signals to internal signals
    noc_io_switch_->timeout_read(noc_timeout_read_);
    noc_io_switch_->timeout_write(noc_timeout_write_);
    smn_io_switch_->timeout(smn_timeout_);
    
    // Combine timeout signals into noc_timeout (3-bit: [0]=NOC-IO read, [1]=NOC-IO write, [2]=SMN-IO)
    SC_METHOD(combine_timeout_process);
    sensitive << noc_timeout_read_ << noc_timeout_write_ << smn_timeout_;
    dont_initialize();
    
    // Connect PCIe Controller interfaces
    // Inbound: PCIe Controller → NOC-PCIE switch target port
    pcie_controller_target.bind(noc_pcie_switch_->pcie_controller_target);
    // Outbound: NOC-PCIE switch initiator port → PCIe Controller
    noc_pcie_switch_->pcie_controller_initiator.bind(pcie_controller_initiator);
    
    // Connect PCIe Controller CII signals to SII block
    sii_block_->cii_hv(pcie_cii_hv);
    sii_block_->cii_hdr_type(pcie_cii_hdr_type);
    sii_block_->cii_hdr_addr(pcie_cii_hdr_addr);
    
    // Connect PCIe Controller clock/reset to SII block
    sii_block_->pcie_core_clk(pcie_core_clk);
    sii_block_->axi_clk(axi_clk);
    sii_block_->reset_n(pcie_controller_reset_n);
    
    // Connect SII output signals to top-level (for connection to PCIe Controller)
    // Use intermediate signals to connect sc_out to sc_out
    sii_block_->app_bus_num(this->app_bus_num_sig_);
    sii_block_->app_dev_num(this->app_dev_num_sig_);
    sii_block_->device_type(this->device_type_sig_);
    sii_block_->sys_int(this->sys_int_sig_);
    pcie_app_bus_num(this->app_bus_num_sig_);
    pcie_app_dev_num(this->app_dev_num_sig_);
    pcie_device_type(this->device_type_sig_);
    pcie_sys_int(this->sys_int_sig_);
    
    // Connect PCIe Controller interrupt inputs to outputs
    // Use intermediate signals to connect sc_in to sc_out
    pcie_flr_request(this->flr_sig_);
    pcie_hot_reset(this->hot_reset_sig_);
    pcie_ras_error(this->ras_error_sig_);
    pcie_dma_completion(this->dma_completion_sig_);
    pcie_misc_int(this->misc_int_sig_);
    function_level_reset(this->flr_sig_);
    hot_reset_requested(this->hot_reset_sig_);
    ras_error(this->ras_error_sig_);
    dma_completion(this->dma_completion_sig_);
    controller_misc_int(this->misc_int_sig_);
}

void KeraunosPcieTile::combine_timeout_process() {
    // Combine individual timeout signals into 3-bit noc_timeout output
    sc_dt::sc_bv<3> timeout_bits;
    timeout_bits[0] = noc_timeout_read_.read();
    timeout_bits[1] = noc_timeout_write_.read();
    timeout_bits[2] = smn_timeout_.read();
    noc_timeout.write(timeout_bits);
}

// Pass-through transport methods for top-level initiator ports
// These receive from internal initiators but have nowhere to forward to
// They just acknowledge the transaction
void KeraunosPcieTile::noc_n_initiator_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // This socket receives from noc_io_switch->noc_n_initiator
    // In a real system, this would forward to external NOC-N
    // In test, the test harness will bind its own initiator here
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int KeraunosPcieTile::noc_n_initiator_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void KeraunosPcieTile::smn_n_initiator_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // This socket receives from smn_io_switch->smn_n_initiator
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int KeraunosPcieTile::smn_n_initiator_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

void KeraunosPcieTile::pcie_controller_initiator_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // This socket receives from noc_pcie_switch->pcie_controller_initiator
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

unsigned int KeraunosPcieTile::pcie_controller_initiator_transport_dbg(tlm::tlm_generic_payload& trans) {
    return 0;
}

} // namespace pcie
} // namespace keraunos

