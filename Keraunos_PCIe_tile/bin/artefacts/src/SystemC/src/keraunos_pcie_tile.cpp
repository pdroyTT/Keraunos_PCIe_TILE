#include "keraunos_pcie_tile.h"

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
    , debug_logging_enabled_(true)
    , outputs_initialized_(false)
{
    SCML2_INFO(FUNCTIONAL_LOG) << "Initializing Keraunos PCIe Tile: " << name << std::endl;
    
    // Register callbacks for target sockets (inbound from external)
    // Initiator sockets don't need register_b_transport - they call outward via ->b_transport()
    noc_n_target.register_b_transport(this, &KeraunosPcieTile::noc_n_target_b_transport);
    smn_n_target.register_b_transport(this, &KeraunosPcieTile::smn_n_target_b_transport);
    pcie_controller_target.register_b_transport(this, &KeraunosPcieTile::pcie_controller_target_b_transport);
    
    // Instantiate internal components using std::make_unique (Modern C++ RAII)
    // No manual delete needed - unique_ptr automatically manages lifetime
    if (debug_logging_enabled_) {
        SCML2_INFO(FUNCTIONAL_LOG) << "Creating internal components..." << std::endl;
    }
    
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
    
    if (debug_logging_enabled_) {
        SCML2_INFO(FUNCTIONAL_LOG) << "All internal components created successfully" << std::endl;
    }
    
    // Set up callback for config register changes
    if (config_reg_) {
        config_reg_->set_change_callback([this]() {
            // When config registers change, update dependent modules
            update_config_dependent_modules();
        });
    }
    
    // Propagate debug logging enable and parent module to child components
    if (config_reg_) {
        config_reg_->set_debug_logging_enabled(debug_logging_enabled_);
        config_reg_->set_parent_module(this);
    }
    if (clock_reset_ctrl_) {
        clock_reset_ctrl_->set_debug_logging_enabled(debug_logging_enabled_);
        clock_reset_ctrl_->set_parent_module(this);
    }
    if (noc_pcie_switch_) {
        noc_pcie_switch_->set_debug_logging_enabled(debug_logging_enabled_);
        noc_pcie_switch_->set_parent_module(this);
    }
    
    // Wire components with function callbacks
    wire_components();
    
    if (debug_logging_enabled_) {
        SCML2_INFO(FUNCTIONAL_LOG) << "Component wiring complete" << std::endl;
    }
    
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
    
    SCML2_INFO(FUNCTIONAL_LOG) << "Elaboration complete" << std::endl;
    
    // NOTE: Output port initialization has been moved into signal_update_process()
    // (guarded by outputs_initialized_) so that ALL writes to output ports
    // originate from the same SC_METHOD process context, preventing E521
    // ("sc_signal<T> cannot have more than one driver").
    
    if (debug_logging_enabled_) {
        SCML2_INFO(CONFIGURATION_INFO) << "Debug logging is ENABLED" << std::endl;
    }
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
    // Spec Outbound_TLBApp_lookup: pa >= (1<<48) → TLBAppOut0, else → TLBAppOut1 (DBI)
    noc_io_switch_->set_tlb_app_output([this](auto& t, auto& d) {
        uint64_t addr = t.get_address();
        if ((addr >> 48) & 0xFFFF) {
            // High address → TLBAppOut0 (16TB pages for regular memory access)
            if (tlb_app_out0_) tlb_app_out0_->process_outbound_traffic(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        } else {
            // Low address → TLBAppOut1 (64KB pages for DBI access)
            if (tlb_app_out1_) tlb_app_out1_->process_outbound_traffic(t, d);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        }
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
    // Spec: TLBAppIn0 has 256 entries across 4 instances (64 each).
    // Dispatch to the correct instance using upper 2 bits of the 8-bit index.
    // index = (addr >> 24) & 0xFF; instance = index >> 6; local = index & 0x3F
    noc_pcie_switch_->set_tlb_app_inbound0_output([this](auto& t, auto& d) {
        uint64_t addr = t.get_address();
        uint8_t full_index = (addr >> 24) & 0xFF;
        uint8_t instance = (full_index >> 6) & 0x3;
        if (instance < tlb_app_in0_.size() && tlb_app_in0_[instance]) {
            tlb_app_in0_[instance]->process_inbound_traffic(t, d);
        } else {
            t.set_response_status(tlm::TLM_OK_RESPONSE);
        }
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
    // TLB Sys In0: translated traffic goes to SMN port (smn_n_initiator), not NOC
    if (tlb_sys_in0_) {
        tlb_sys_in0_->set_translated_output([this](auto& t, auto& d) {
            if (smn_io_switch_) smn_io_switch_->route_from_smn(t, d);
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
    // Outbound TLB translated outputs: pass AxUSER (TLB ATTR) to NOC-PCIE
    // for BME qualification per Table 33, Section 2.5.8.1
    if (tlb_sys_out0_) {
        tlb_sys_out0_->set_translated_output([this](auto& t, auto& d, const auto& attr) {
            // AxUSER bit[21] = DBI Access Indicator (Table 24/25).
            // DBI transactions must reach PCIe_EP0.AXI_DBI (at 0x44000000
            // on smn_n_initiator), NOT AXI_Slave via pcie_controller_initiator.
            bool is_dbi = attr[21].to_bool();
            if (is_dbi) {
                uint64_t orig = t.get_address();
                t.set_address(0x44000000ULL | (orig & 0x3FFFFFULL));
                smn_n_initiator->b_transport(t, d);
                t.set_address(orig);
            } else if (noc_pcie_switch_) {
                noc_pcie_switch_->route_to_pcie(t, d, attr);
            } else {
                t.set_response_status(tlm::TLM_OK_RESPONSE);
            }
        });
    }
    if (tlb_app_out0_) {
        tlb_app_out0_->set_translated_output([this](auto& t, auto& d, const auto& attr) {
            if (noc_pcie_switch_) {
                if (debug_logging_enabled_) {
                    uint32_t tlp_type = 0;
                    for (int i = 0; i < 5; i++) { if (attr[i].to_bool()) tlp_type |= (1u << i); }
                    bool dbi = attr[21].to_bool();
                    
                    SCML2_INFO(FUNCTIONAL_LOG)
                        << "TLBAppOut0 -> PCIe: addr=0x" << std::hex << t.get_address() << std::dec
                        << " BME=" << noc_pcie_switch_->get_bus_master_enable()
                        << " EP=" << noc_pcie_switch_->get_controller_is_ep()
                        << " TLP_TYPE=0x" << std::hex << tlp_type << std::dec
                        << " DBI=" << dbi
                        << " cmd=" << (t.get_command() == tlm::TLM_READ_COMMAND ? "READ" : "WRITE")
                        << std::endl;
                }
                
                noc_pcie_switch_->route_to_pcie(t, d, attr);
                
                if (debug_logging_enabled_) {
                    SCML2_INFO(FUNCTIONAL_LOG) 
                        << "TLBAppOut0 routing response: " << t.get_response_status() << std::endl;
                }
            }
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
    if (tlb_app_out1_) {
        tlb_app_out1_->set_translated_output([this](auto& t, auto& d, const auto& attr) {
            if (noc_pcie_switch_) noc_pcie_switch_->route_to_pcie(t, d, attr);
            else t.set_response_status(tlm::TLM_OK_RESPONSE);
        });
    }
    
    // Wire SII device_type callback so APB writes to CORE_CONTROL
    // immediately propagate controller_is_ep_ to NocPcieSwitch (BME logic).
    if (sii_block_) {
        sii_block_->set_device_type_callback([this](bool is_rp) {
            if (noc_pcie_switch_) noc_pcie_switch_->set_controller_is_ep(!is_rp);
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
    if (debug_logging_enabled_) {
        SCML2_INFO(FUNCTIONAL_LOG) 
            << "NOC_N target received transaction: addr=0x" << std::hex << trans.get_address() 
            << " cmd=" << (trans.get_command() == tlm::TLM_READ_COMMAND ? "READ" : "WRITE")
            << std::dec << " len=" << trans.get_data_length() << std::endl;
    }
    
    if (noc_io_switch_) {
        noc_io_switch_->route_from_noc(trans, delay);
        
        if (debug_logging_enabled_) {
            SCML2_INFO(FUNCTIONAL_LOG) 
                << "NOC_N routing complete: response=" << trans.get_response_status() << std::endl;
        }
        
        if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        SCML2_WARNING(GENERIC_WARNING) << "NOC_N switch not initialized" << std::endl;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void KeraunosPcieTile::smn_n_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    uint32_t len  = trans.get_data_length();
    
    if (debug_logging_enabled_) {
        SCML2_INFO(FUNCTIONAL_LOG) 
            << "SMN_N target received transaction: addr=0x" << std::hex << addr 
            << " cmd=" << (trans.get_command() == tlm::TLM_READ_COMMAND ? "READ" : "WRITE")
            << std::dec << " len=" << len << std::endl;
    }

    // Split 8-byte (64-bit) transactions into two 4-byte transactions.
    // Some bus interconnects (e.g. Imperas ISS TLM bridge) generate 8-byte
    // TLM transactions for RISC-V sd/ld instructions.  Internal sub-blocks
    // may only handle 4-byte accesses, so we split here.
    if (len == 8 && smn_io_switch_) {
        uint8_t* data_ptr = trans.get_data_ptr();

        // First 4 bytes (lower half)
        trans.set_data_length(4);
        trans.set_streaming_width(4);
        smn_io_switch_->route_from_smn(trans, delay);
        if (trans.get_response_status() != tlm::TLM_OK_RESPONSE &&
            trans.get_response_status() != tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_data_length(len);
            trans.set_streaming_width(len);
            return;
        }

        // Second 4 bytes (upper half)
        trans.set_address(addr + 4);
        trans.set_data_ptr(data_ptr + 4);
        smn_io_switch_->route_from_smn(trans, delay);

        // Restore original transaction fields
        trans.set_address(addr);
        trans.set_data_ptr(data_ptr);
        trans.set_data_length(len);
        trans.set_streaming_width(len);

        if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
        return;
    }
    
    if (smn_io_switch_) {
        smn_io_switch_->route_from_smn(trans, delay);
        
        if (debug_logging_enabled_) {
            SCML2_INFO(FUNCTIONAL_LOG) 
                << "SMN_N routing complete: response=" << trans.get_response_status() << std::endl;
        }
        
        if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        SCML2_WARNING(GENERIC_WARNING) << "SMN_N switch not initialized" << std::endl;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void KeraunosPcieTile::pcie_controller_target_b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
    uint64_t addr = trans.get_address();
    const char* cmd_str = (trans.get_command() == tlm::TLM_READ_COMMAND) ? "READ" : "WRITE";
    
    if (debug_logging_enabled_) {
        SCML2_INFO(FUNCTIONAL_LOG) 
            << "PCIe Controller transaction entry: addr=0x" << std::hex << addr << std::dec
            << " cmd=" << cmd_str 
            << " len=" << trans.get_data_length()
            << " time=" << sc_core::sc_time_stamp() << std::endl;
    }

    if (noc_pcie_switch_) {
        noc_pcie_switch_->route_from_pcie(trans, delay);
        
        if (debug_logging_enabled_) {
            SCML2_INFO(FUNCTIONAL_LOG) 
                << "PCIe Controller transaction exit: response=" << trans.get_response_status()
                << " time=" << sc_core::sc_time_stamp() << std::endl;
        }
        
        if (trans.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE) {
            trans.set_response_status(tlm::TLM_OK_RESPONSE);
        }
    } else {
        SCML2_ERROR(FUNCTIONAL_ERROR) << "PCIe switch not initialized" << std::endl;
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
}

void KeraunosPcieTile::update_config_dependent_modules() {
    // Update modules that depend on config register values
    if (config_reg_) {
        bool sys_ready = config_reg_->get_system_ready();
        bool out_enable = config_reg_->get_pcie_outbound_app_enable();
        bool in_enable = config_reg_->get_pcie_inbound_app_enable();
        
        if (debug_logging_enabled_) {
            SCML2_INFO(CONFIGURATION_INFO) 
                << "Config registers updated: system_ready=" << sys_ready
                << " outbound_enable=" << out_enable
                << " inbound_enable=" << in_enable << std::endl;
        }
        
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
    // First-run initialization: set all output ports to safe defaults.
    // Done here (not in end_of_elaboration) so that every write to an
    // output port originates from this single SC_METHOD, preventing E521.
    if (!outputs_initialized_) {
        outputs_initialized_ = true;
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
        noc_timeout.write(0u);
        return;
    }
    
    // Update internal component states from input signals (with null safety)
    bool cold_rst = cold_reset_n.read();
    bool warm_rst = warm_reset_n.read();
    bool isolate = isolate_req.read();
    
    if (clock_reset_ctrl_) {
        clock_reset_ctrl_->set_cold_reset_n(cold_rst);
        clock_reset_ctrl_->set_warm_reset_n(warm_rst);
        clock_reset_ctrl_->set_isolate_req(isolate);
    }
    
    if (config_reg_) {
        config_reg_->set_isolate_req(isolate);
    }
    
    if (noc_pcie_switch_) {
        noc_pcie_switch_->set_isolate_req(isolate);
        
        if (!cold_rst) {
            if (debug_logging_enabled_) {
                SCML2_INFO(CONFIGURATION_INFO) 
                    << "Cold reset asserted - restoring BME to default (enabled)" << std::endl;
            }
            noc_pcie_switch_->set_bus_master_enable(true);
        }
    }
    if (noc_io_switch_) noc_io_switch_->set_isolate_req(isolate_req.read());
    if (smn_io_switch_) smn_io_switch_->set_isolate_req(isolate_req.read());
    
    if (sii_block_) {
        sii_block_->set_cii_hv(pcie_cii_hv.read());
        sii_block_->set_cii_hdr_type(pcie_cii_hdr_type.read());
        sii_block_->set_cii_hdr_addr(pcie_cii_hdr_addr.read());
        sii_block_->set_reset_n(pcie_controller_reset_n.read());

        sii_block_->update();

        pcie_app_bus_num.write(sii_block_->get_app_bus_num());
        pcie_app_dev_num.write(sii_block_->get_app_dev_num());
        bool is_rp = sii_block_->get_device_type();
        pcie_device_type.write(is_rp);
        pcie_sys_int.write(sii_block_->get_sys_int());
        config_update.write(sii_block_->get_config_int());
        
        if (noc_pcie_switch_) {
            bool prev_ep = noc_pcie_switch_->get_controller_is_ep();
            bool new_ep = !is_rp;
            if (prev_ep != new_ep && debug_logging_enabled_) {
                SCML2_INFO(CONFIGURATION_INFO) 
                    << "PCIe controller mode changed to " << (new_ep ? "Endpoint" : "Root Port") << std::endl;
            }
            noc_pcie_switch_->set_controller_is_ep(new_ep);
        }
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
    unsigned int timeout_val = 0;
    if (noc_io_switch_) {
        if (noc_io_switch_->get_timeout_read())  timeout_val |= (1u << 0);
        if (noc_io_switch_->get_timeout_write()) timeout_val |= (1u << 1);
    }
    if (smn_io_switch_) {
        if (smn_io_switch_->get_timeout()) timeout_val |= (1u << 2);
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
