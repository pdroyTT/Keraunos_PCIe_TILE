#ifndef TB_MONITOR_H
#define TB_MONITOR_H

#include "tb_common.h"
#include <scml2.h>
#include <systemc>
#include <tlm>
#include <vector>
#include <cstring>

namespace keraunos {
namespace pcie {
namespace tb {

/**
 * Transaction Monitor
 * 
 * Monitors transactions on a socket and records them
 */
class TransactionMonitor : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<TransactionMonitor, 64> target_socket_;
    
    SC_HAS_PROCESS(TransactionMonitor);
    
    TransactionMonitor(sc_core::sc_module_name name)
        : sc_module(name)
        , target_socket_("target_socket")
    {
        target_socket_.register_b_transport(this, &TransactionMonitor::b_transport);
    }
    
    const std::vector<TransactionRecord>& get_transactions() const {
        return records_;
    }
    
    void clear() { records_.clear(); }
    
    // Check if transaction with specific address occurred
    bool has_transaction(uint64_t addr) const {
        for (const auto& rec : records_) {
            if (rec.addr == addr) {
                return true;
            }
        }
        return false;
    }
    
    // Get last transaction
    const TransactionRecord* get_last_transaction() const {
        if (records_.empty()) {
            return nullptr;
        }
        return &records_.back();
    }
    
protected:
    // According to VZ_SCMLRef.md and tlm_utils::simple_target_socket, b_transport returns void
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
        TransactionRecord record;
        record.addr = trans.get_address();
        record.is_read = (trans.get_command() == tlm::TLM_READ_COMMAND);
        record.status = trans.get_response_status();
        record.timestamp = sc_core::sc_time_stamp();
        
        if (record.is_read && trans.get_data_length() >= 4) {
            memcpy(&record.data, trans.get_data_ptr(), 4);
        } else if (!record.is_read && trans.get_data_length() >= 4) {
            memcpy(&record.data, trans.get_data_ptr(), 4);
        }
        
        records_.push_back(record);
        
        // Forward transaction (passthrough monitor) - response status already set
        // Note: This is a monitor, so we don't modify the transaction
    }
    
private:
    std::vector<TransactionRecord> records_;
};

} // namespace tb
} // namespace pcie
} // namespace keraunos

#endif // TB_MONITOR_H

