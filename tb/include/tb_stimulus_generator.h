#ifndef TB_STIMULUS_GENERATOR_H
#define TB_STIMULUS_GENERATOR_H

#include "../include/keraunos_pcie_tlb_common.h"
#include "tb_common.h"
#include <scml2.h>
#include <systemc>
#include <tlm>
#include <cstdint>
#include <vector>
#include <random>
#include <cstring>

namespace keraunos {
namespace pcie {
namespace tb {

/**
 * Stimulus Generator for TLB Tests
 * 
 * Generates TLM transactions for testing TLB modules
 */
class TlbStimulusGenerator : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(TlbStimulusGenerator);
    
    TlbStimulusGenerator(sc_core::sc_module_name name)
        : sc_module(name)
        , initiator_socket_("initiator_socket")
    {
    }
    
    // Generate TLB test transaction
    void generate_tlb_test(uint64_t addr, bool is_read = true, 
                           unsigned int length = 4) {
        tlm::tlm_generic_payload* trans;
        
        if (is_read) {
            trans = TransactionGenerator::create_read_transaction(addr, length);
        } else {
            uint32_t data = 0xDEADBEEF;
            trans = TransactionGenerator::create_write_transaction(
                addr, reinterpret_cast<uint8_t*>(&data), length);
        }
        
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        initiator_socket_->b_transport(*trans, delay);
        
        TransactionGenerator::release_transaction(trans);
    }
    
    // Generate address range test
    void generate_address_range_test(uint64_t base_addr, uint64_t end_addr,
                                     uint64_t step, bool is_read = true) {
        for (uint64_t addr = base_addr; addr < end_addr; addr += step) {
            generate_tlb_test(addr, is_read);
            wait(sc_core::sc_time(10, sc_core::SC_NS));
        }
    }
    
    // Generate error test (invalid address)
    void generate_error_test(uint64_t invalid_addr) {
        generate_tlb_test(invalid_addr, true);
    }
    
    // Generate random transactions
    void generate_random_test(uint64_t base_addr, uint64_t range,
                            unsigned int count, std::mt19937& rng) {
        std::uniform_int_distribution<uint64_t> addr_dist(base_addr, 
                                                          base_addr + range);
        std::uniform_int_distribution<int> cmd_dist(0, 1);
        
        for (unsigned int i = 0; i < count; i++) {
            uint64_t addr = addr_dist(rng);
            bool is_read = (cmd_dist(rng) == 0);
            generate_tlb_test(addr, is_read);
            wait(sc_core::sc_time(10, sc_core::SC_NS));
        }
    }
    
    scml2::initiator_socket<64> initiator_socket_;
};

/**
 * APB Stimulus Generator for Configuration Access
 */
class ApbStimulusGenerator : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(ApbStimulusGenerator);
    
    ApbStimulusGenerator(sc_core::sc_module_name name)
        : sc_module(name)
        , initiator_socket_("initiator_socket")
    {
    }
    
    // Write to APB register
    void write_register(uint32_t addr, uint32_t data) {
        tlm::tlm_generic_payload* trans = 
            TransactionGenerator::create_write_transaction(
                addr, reinterpret_cast<uint8_t*>(&data), 4);
        
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        initiator_socket_->b_transport(*trans, delay);
        
        TransactionGenerator::release_transaction(trans);
    }
    
    // Read from APB register
    uint32_t read_register(uint32_t addr) {
        tlm::tlm_generic_payload* trans = 
            TransactionGenerator::create_read_transaction(addr, 4);
        
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        initiator_socket_->b_transport(*trans, delay);
        
        uint32_t data = 0;
        if (trans->get_response_status() == tlm::TLM_OK_RESPONSE) {
            memcpy(&data, trans->get_data_ptr(), 4);
        }
        
        TransactionGenerator::release_transaction(trans);
        return data;
    }
    
    // Configure TLB entry via APB
    void configure_tlb_entry(uint32_t base_addr, uint8_t index, 
                            const TlbEntry& entry) {
        uint32_t entry_offset = index * 64;  // 64 bytes per entry
        uint32_t entry_addr = base_addr + entry_offset;
        
        // Write entry data (simplified - actual implementation would write 64 bytes)
        // For testing, we'll write key fields
        uint64_t entry_data[8] = {0};
        entry_data[0] = entry.valid ? 1ULL : 0ULL;
        entry_data[1] = entry.addr;
        
        // Write entry (simplified)
        write_register(entry_addr, static_cast<uint32_t>(entry_data[0]));
        write_register(entry_addr + 4, static_cast<uint32_t>(entry_data[1]));
        write_register(entry_addr + 8, static_cast<uint32_t>(entry_data[1] >> 32));
    }
    
    scml2::initiator_socket<32> initiator_socket_;
};

} // namespace tb
} // namespace pcie
} // namespace keraunos

#endif // TB_STIMULUS_GENERATOR_H

