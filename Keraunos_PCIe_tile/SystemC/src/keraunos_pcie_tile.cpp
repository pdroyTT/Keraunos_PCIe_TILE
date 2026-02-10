#include "keraunos_pcie_tile.h"
#include <fstream>
#include <cstdio>

namespace keraunos {
namespace pcie {

KeraunosPcieTile::KeraunosPcieTile(sc_core::sc_module_name name)
    : sc_module(name)
    , noc_n_target("noc_n_target")
    , noc_n_initiator("noc_n_initiator")
    , smn_n_target("smn_n_target")
    , smn_n_initiator("smn_n_initiator")
    , pcie_controller_target("pcie_controller_target")
    , pcie_controller_initiator("pcie_controller_initiator")
{
    // Register callbacks for target sockets (inbound from external)
    // Initiator sockets don't need register_b_transport - they call outward via ->b_transport()
    noc_n_target.register_b_transport(this, &KeraunosPcieTile::noc_n_target_b_transport);
    smn_n_target.register_b_transport(this, &KeraunosPcieTile::smn_n_target_b_transport);
    pcie_controller_target.register_b_transport(this, &KeraunosPcieTile::pcie_controller_target_b_transport);
    
    // Instantiate internal components using std::make_unique (Modern C++ RAII)
    // No manual delete needed - unique_ptr automatically manages lifetime
    noc_pcie_switch_ = std::make_unique<NocPcieSwitch>();
    noc_io_switch_ = std::make_unique<NocIoSwitch>();
    smn_io_switch_ = std::make_unique<SmnIoSwitch>();
    tlb_sys_in0_ = std::make_unique<TLBSysIn0>();
    for (size_t i = 0; i < tlb_app_in0_.size(); i++) {
        tlb_app_in0_[i] = std::make_unique<TLBAppIn0>(static_cast<uint8_t>(i));
    }
    tlb_app_in1_ = std::make_unique<TLBAppIn1>();
    tlb_sys_out0_ = std::make_unique<TLBSysOut0>();
    tlb_app_out0_ = std::make_unique<TLBAppOut0>();
    tlb_app_out1_ = std::make_unique<TLBAppOut1>();
    msi_relay_ = std::make_unique<MsiRelayUnit>(16);
    sii_block_ = std::make_unique<SiiBlock>();
    config_reg_ = std::make_unique<ConfigRegBlock>();
    clock_reset_ctrl_ = std::make_unique<ClockResetControl>();
    pll_cgm_ = std::make_unique<PllCgm>();
    pcie_phy_ = std::make_unique<PciePhy>();
    
    // Set up callback for config register changes
    if (config_reg_) {
        config_reg_->set_change_callback([this]() {
            // When config registers change, update dependent modules
            update_config_dependent_modules();
        });
    }
    
    // Wire components with function callbacks
    wire_components();
    
    // Register signal update process
    SC_METHOD(signal_update_process);
    sensitive << cold_reset_n << warm_reset_n << isolate_req << pcie_core_clk << axi_clk
              << pcie_cii_hv << pcie_cii_hdr_type << pcie_cii_hdr_addr
              << pcie_controller_reset_n << pcie_flr_request << pcie_hot_reset
              << pcie_ras_error << pcie_dma_completion << pcie_misc_int;
}

KeraunosPcieTile::~KeraunosPcieTile() {
    // No manual delete needed - std::unique_ptr automatically manages memory (RAII)
    // This eliminates all potential memory leaks from exceptions or early returns
}

void KeraunosPcieTile::end_of_elaboration() {
    sc_module::end_of_elaboration();
    // Initialize outputs
    pcie_app_bus_num.write(0);
    pcie_app_dev_num.write(0);
    pcie_device_type.write(false);
    pcie_sys_int.write(false);
    function_level_reset.write(false);
    hot_reset_requested.write(false);
    config_update.write(false);
    ras_error.write(false);
    dma_completion.write(false);
    controller_misc_int.write(false);
    noc_timeout.write(sc_dt::sc_bv<3>(0));
}

void KeraunosPcieTile::wire_components() {
    // Wire NOC-IO Switch (with null safety checks)
    // Forward NOC outbound traffic through the initiator socket to external (testbench)
    noc_io_switch_->set_noc_n_output([this](auto& t, auto& d) {
        noc_n_initiator->b_transport(t, d);
    });
    noc_io_switch_->set_msi_relay_output([this](auto& t, auto& d) {
        if (msi_relay_) msi_relay_->process_msi_input(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_io_switch_->set_tlb_app_output([this](auto& t, auto& d) {
        if (tlb_app_out0_) tlb_app_out0_->process_outbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    
    // Wire SMN-IO Switch
    // Forward SMN outbound traffic through the initiator socket to external (testbench)
    smn_io_switch_->set_smn_n_output([this](auto& t, auto& d) {
        smn_n_initiator->b_transport(t, d);
    });
    // Config reg: SMN-IO switch already computes offset from Config Reg Block base (0x18040000)
    smn_io_switch_->set_config_reg_output([this](auto& t, auto& d) {
        if (config_reg_) {
            config_reg_->process_apb_access(t, d);
        } else {
            t.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    });
    smn_io_switch_->set_msi_relay_cfg_output([this](auto& t, auto& d) {
        msi_relay_->process_csr_access(t, d);
    });
    smn_io_switch_->set_sii_config_output([this](auto& t, auto& d) {
        sii_block_->process_apb_access(t, d);
    });
    smn_io_switch_->set_serdes_apb_output([this](auto& t, auto& d) {
        pcie_phy_->process_apb_access(t, d);
    });
    smn_io_switch_->set_serdes_ahb_output([this](auto& t, auto& d) {
        pcie_phy_->process_ahb_access(t, d);
    });
    smn_io_switch_->set_tlb_sys_in0_cfg_output([this](auto& t, auto& d) {
        if (tlb_sys_in0_) tlb_sys_in0_->process_config_access(t, d);  // Null safety
    });
    for (size_t i = 0; i < tlb_app_in0_.size(); i++) {
        smn_io_switch_->set_tlb_app_in0_cfg_output(static_cast<int>(i), [this, i](auto& t, auto& d) {
            if (tlb_app_in0_[i]) tlb_app_in0_[i]->process_config_access(t, d);  // Null safety
        });
    }
    smn_io_switch_->set_tlb_app_in1_cfg_output([this](auto& t, auto& d) {
        if (tlb_app_in1_) tlb_app_in1_->process_config_access(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    smn_io_switch_->set_tlb_sys_out0_cfg_output([this](auto& t, auto& d) {
        if (tlb_sys_out0_) tlb_sys_out0_->process_config_access(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    smn_io_switch_->set_tlb_app_out0_cfg_output([this](auto& t, auto& d) {
        if (tlb_app_out0_) tlb_app_out0_->process_config_access(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    smn_io_switch_->set_tlb_app_out1_cfg_output([this](auto& t, auto& d) {
        if (tlb_app_out1_) tlb_app_out1_->process_config_access(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    smn_io_switch_->set_tlb_sys_inbound_output([this](auto& t, auto& d) {
        if (tlb_sys_in0_) tlb_sys_in0_->process_inbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    smn_io_switch_->set_tlb_sys_outbound_output([this](auto& t, auto& d) {
        if (tlb_sys_out0_) tlb_sys_out0_->process_outbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    
    // Wire NOC-PCIE Switch (with null safety checks)
    noc_pcie_switch_->set_tlb_app_inbound0_output([this](auto& t, auto& d) {
        if (tlb_app_in0_[0]) tlb_app_in0_[0]->process_inbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_tlb_app_inbound1_output([this](auto& t, auto& d) {
        if (tlb_app_in1_) tlb_app_in1_->process_inbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_tlb_sys_inbound_output([this](auto& t, auto& d) {
        if (tlb_sys_in0_) tlb_sys_in0_->process_inbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_tlb_app_out0_output([this](auto& t, auto& d) {
        if (tlb_app_out0_) tlb_app_out0_->process_outbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_tlb_app_out1_output([this](auto& t, auto& d) {
        if (tlb_app_out1_) tlb_app_out1_->process_outbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_tlb_sys_out0_output([this](auto& t, auto& d) {
        if (tlb_sys_out0_) tlb_sys_out0_->process_outbound_traffic(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_noc_io_output([this](auto& t, auto& d) {
        if (noc_io_switch_) noc_io_switch_->route_from_noc(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_smn_io_output([this](auto& t, auto& d) {
        if (smn_io_switch_) smn_io_switch_->route_from_smn(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    // Forward PCIe outbound traffic through the initiator socket to external (testbench)
    noc_pcie_switch_->set_pcie_controller_output([this](auto& t, auto& d) {
        pcie_controller_initiator->b_transport(t, d);
    });
    noc_pcie_switch_->set_msi_relay_output([this](auto& t, auto& d) {
        if (msi_relay_) msi_relay_->process_msi_input(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    noc_pcie_switch_->set_config_reg_output([this](auto& t, auto& d) {
        if (config_reg_) config_reg_->process_apb_access(t, d);
        else t.set_response_status(tlm::TLM_OK_RESPONSE);
    });
    
    // Wire TLB outputs (with null safety checks)
    if (tlb_sys_in0_) {
        tlb_sys_in0_->set_translated_output([this](auto& t, auto& d) {
            if (noc_io_switch_) noc_io_switch_->route_from_tlb(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
    for (size_t i = 0; i < tlb_app_in0_.size(); i++) {
        if (tlb_app_in0_[i]) {
            tlb_app_in0_[i]->set_translated_output([this](auto& t, auto& d) {
                if (noc_io_switch_) noc_io_switch_->route_from_tlb(t, d);
                else t.set_response_status(tlm::TLM_OK_RESPONSE);
            });
        }
    }
    if (tlb_app_in1_) {
        tlb_app_in1_->set_translated_output([this](auto& t, auto& d) {
            if (noc_io_switch_) noc_io_switch_->route_from_tlb(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
    if (tlb_sys_out0_) {
        tlb_sys_out0_->set_translated_output([this](auto& t, auto& d) {
            if (noc_pcie_switch_) noc_pcie_switch_->route_to_pcie(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
    if (tlb_app_out0_) {
        tlb_app_out0_->set_translated_output([this](auto& t, auto& d) {
            if (noc_pcie_switch_) noc_pcie_switch_->route_to_pcie(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
    if (tlb_app_out1_) {
        tlb_app_out1_->set_translated_output([this](auto& t, auto& d) {
            if (noc_pcie_switch_) noc_pcie_switch_->route_to_pcie(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
    
    // Wire MSI Relay output (with null safety)
    if (msi_relay_) {
        msi_relay_->set_msi_output_callback([this](auto& t, auto& d) {
            if (noc_io_switch_) noc_io_switch_->route_from_noc(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
}

// Top-level socket transport methods (with null safety)
void KeraunosPcieTile::noc_n_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    if (noc_io_switch_) {
        noc_io_switch_->route_from_noc(trans, delay);
        if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void KeraunosPcieTile::smn_n_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    
    if (smn_io_switch_) {
        smn_io_switch_->route_from_smn(trans, delay);
        if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void KeraunosPcieTile::pcie_controller_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    // #region agent log
    {
        std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug.log", std::ios::app);
        char buf[512];
        uint64_t addr = trans.get_address();
        snprintf(buf, sizeof(buf), "{\"location\":\"keraunos_pcie_tile.cpp:pcie_b_transport\",\"message\":\"PCIe DUT ENTRY\",\"data\":{\"addr\":\"0x%lx\",\"cmd\":\"%s\",\"has_switch\":%s},\"timestamp\":%ld,\"hypothesisId\":\"DUT_ENTRY\"}\n",
                 addr, (trans.get_command() == tlm::TLM_READ_COMMAND ? "READ" : "WRITE"),
                 noc_pcie_switch_ ? "true" : "false", sc_core::sc_time_stamp().value());
        f << buf;
    }
    // #endregion

    if (noc_pcie_switch_) {
        noc_pcie_switch_->route_from_pcie(trans, delay);
        if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    // #region agent log
    {
        std::ofstream f("/localdev/pdroy/keraunos_pcie_workspace/.cursor/debug.log", std::ios::app);
        char buf[256];
        snprintf(buf, sizeof(buf), "{\"location\":\"keraunos_pcie_tile.cpp:pcie_b_transport\",\"message\":\"PCIe DUT EXIT\",\"data\":{\"response\":%d},\"timestamp\":%ld,\"hypothesisId\":\"DUT_EXIT\"}\n",
                 trans.get_response_status(), sc_core::sc_time_stamp().value());
        f << buf;
    }
    // #endregion
}

void KeraunosPcieTile::update_config_dependent_modules() {
    // Update modules that depend on config register values
    if (config_reg_) {
        bool sys_ready = config_reg_->get_system_ready();
        bool out_enable = config_reg_->get_pcie_outbound_app_enable();
        bool in_enable = config_reg_->get_pcie_inbound_app_enable();
        
        if (noc_pcie_switch_) {
            noc_pcie_switch_->set_system_ready(sys_ready);
            noc_pcie_switch_->set_pcie_outbound_app_enable(out_enable);
            noc_pcie_switch_->set_pcie_inbound_app_enable(in_enable);
        }
        
        if (tlb_sys_in0_) {
            tlb_sys_in0_->set_system_ready(sys_ready);
        }
    }
}

void KeraunosPcieTile::signal_update_process() {
    // Update internal component states from input signals (with null safety)
    if (clock_reset_ctrl_) {
        clock_reset_ctrl_->set_cold_reset_n(cold_reset_n.read());
        clock_reset_ctrl_->set_warm_reset_n(warm_reset_n.read());
        clock_reset_ctrl_->set_isolate_req(isolate_req.read());
    }
    
    if (config_reg_) {
        config_reg_->set_isolate_req(isolate_req.read());
        // Config-dependent updates now happen via callback
    }
    
    if (noc_pcie_switch_) noc_pcie_switch_->set_isolate_req(isolate_req.read());
    if (noc_io_switch_) noc_io_switch_->set_isolate_req(isolate_req.read());
    if (smn_io_switch_) smn_io_switch_->set_isolate_req(isolate_req.read());
    
    if (sii_block_) {
        // Set CII inputs from PCIe controller signals
        sii_block_->set_cii_hv(pcie_cii_hv.read());
        sii_block_->set_cii_hdr_type(pcie_cii_hdr_type.read());
        sii_block_->set_cii_hdr_addr(pcie_cii_hdr_addr.read());
        sii_block_->set_reset_n(pcie_controller_reset_n.read());

        // Process CII tracking, cfg_modified update, interrupt generation
        sii_block_->update();

        // Drive outputs from SII
        pcie_app_bus_num.write(sii_block_->get_app_bus_num());
        pcie_app_dev_num.write(sii_block_->get_app_dev_num());
        pcie_device_type.write(sii_block_->get_device_type());
        pcie_sys_int.write(sii_block_->get_sys_int());
        config_update.write(sii_block_->get_config_int());
    }
    
    if (pll_cgm_ && clock_reset_ctrl_) {
        pll_cgm_->set_reset_n(clock_reset_ctrl_->get_pcie_sii_reset_ctrl());
    }
    if (pcie_phy_ && clock_reset_ctrl_) {
        pcie_phy_->set_reset_n(clock_reset_ctrl_->get_pcie_reset_ctrl());
    }
    
    // Pass through PCIe controller interrupts
    function_level_reset.write(pcie_flr_request.read());
    hot_reset_requested.write(pcie_hot_reset.read());
    ras_error.write(pcie_ras_error.read());
    dma_completion.write(pcie_dma_completion.read());
    controller_misc_int.write(pcie_misc_int.read());
    
    // Combine timeout signals (with null safety)
    sc_dt::sc_bv<3> timeout_val;
    if (noc_io_switch_) {
        timeout_val[0] = noc_io_switch_->get_timeout_read();
        timeout_val[1] = noc_io_switch_->get_timeout_write();
    }
    if (smn_io_switch_) {
        timeout_val[2] = smn_io_switch_->get_timeout();
    }
    noc_timeout.write(timeout_val);
    
    // Trigger MSI processing (with null safety)
    if (msi_relay_) {
        msi_relay_->set_msix_enable(msix_enable_.read());
        msi_relay_->set_msix_mask(msix_mask_.read());
        msi_relay_->set_interrupt_pending(setip_.read().to_uint());
        msi_relay_->process_pending_msis();
    }
}

} // namespace pcie
} // namespace keraunos
