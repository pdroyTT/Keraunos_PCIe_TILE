#ifndef TB_COMMON_H
#define TB_COMMON_H

#include <systemc>
#include <tlm>
#include <scml2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>

namespace keraunos {
namespace pcie {
namespace tb {

// Test result enumeration
enum class TestResult {
    PASS,
    FAIL,
    SKIP
};

// Test case base class
class TestCase {
public:
    TestCase(const std::string& name) : name_(name), result_(TestResult::SKIP) {}
    virtual ~TestCase() {}
    
    virtual void run() = 0;
    virtual void setup() {}
    virtual void teardown() {}
    
    const std::string& name() const { return name_; }
    TestResult result() const { return result_; }
    
protected:
    std::string name_;
    TestResult result_;
    
    void pass() { result_ = TestResult::PASS; }
    void fail(const std::string& reason) {
        result_ = TestResult::FAIL;
        std::cout << "FAIL: " << name_ << " - " << reason << std::endl;
    }
    
    void check(bool condition, const std::string& message) {
        if (!condition) {
            fail(message);
        }
    }
};

// Transaction generator helper
class TransactionGenerator {
public:
    static tlm::tlm_generic_payload* create_read_transaction(
        uint64_t addr, unsigned int length = 4) {
        tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload();
        uint8_t* data = new uint8_t[length];
        
        trans->set_command(tlm::TLM_READ_COMMAND);
        trans->set_address(addr);
        trans->set_data_ptr(data);
        trans->set_data_length(length);
        trans->set_streaming_width(length);
        trans->set_byte_enable_ptr(nullptr);
        trans->set_dmi_allowed(false);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        
        return trans;
    }
    
    static tlm::tlm_generic_payload* create_write_transaction(
        uint64_t addr, const uint8_t* data, unsigned int length = 4) {
        tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload();
        uint8_t* trans_data = new uint8_t[length];
        memcpy(trans_data, data, length);
        
        trans->set_command(tlm::TLM_WRITE_COMMAND);
        trans->set_address(addr);
        trans->set_data_ptr(trans_data);
        trans->set_data_length(length);
        trans->set_streaming_width(length);
        trans->set_byte_enable_ptr(nullptr);
        trans->set_dmi_allowed(false);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        
        return trans;
    }
    
    static void release_transaction(tlm::tlm_generic_payload* trans) {
        if (trans) {
            delete[] trans->get_data_ptr();
            delete trans;
        }
    }
};

// TransactionMonitor is defined in tb_monitor.h as a SystemC module
// This was a duplicate definition - removed to avoid conflicts
// Use the TransactionMonitor from tb_monitor.h instead

// Transaction record structure (shared with TransactionMonitor)
struct TransactionRecord {
    uint64_t addr;
    bool is_read;
    uint32_t data;
    tlm::tlm_response_status status;
    sc_core::sc_time timestamp;
    
    TransactionRecord() : addr(0), is_read(false), data(0), 
                          status(tlm::TLM_INCOMPLETE_RESPONSE),
                          timestamp(sc_core::SC_ZERO_TIME) {}
};

// Helper function for recording transactions (if needed outside TransactionMonitor)
inline void record_transaction_helper(std::vector<TransactionRecord>& records, const tlm::tlm_generic_payload& trans) {
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
    
    records.push_back(record);
}

// Scoreboard for comparing DUT vs reference
class Scoreboard {
public:
    void compare_translation(uint64_t input_addr, uint64_t expected_addr,
                            uint64_t actual_addr, bool& match) {
        match = (expected_addr == actual_addr);
        if (!match) {
            mismatches_.push_back({input_addr, expected_addr, actual_addr});
        }
    }
    
    void compare_response(tlm::tlm_response_status expected,
                         tlm::tlm_response_status actual, bool& match) {
        match = (expected == actual);
        if (!match) {
            response_mismatches_.push_back({expected, actual});
        }
    }
    
    bool has_mismatches() const {
        return !mismatches_.empty() || !response_mismatches_.empty();
    }
    
    void print_mismatches() const {
        for (const auto& m : mismatches_) {
            std::cout << "Address mismatch: input=0x" << std::hex << m.input
                     << ", expected=0x" << m.expected
                     << ", actual=0x" << m.actual << std::dec << std::endl;
        }
        for (const auto& m : response_mismatches_) {
            std::cout << "Response mismatch: expected=" << m.expected
                     << ", actual=" << m.actual << std::endl;
        }
    }
    
    void clear() {
        mismatches_.clear();
        response_mismatches_.clear();
    }
    
private:
    struct AddressMismatch {
        uint64_t input, expected, actual;
    };
    struct ResponseMismatch {
        tlm::tlm_response_status expected, actual;
    };
    
    std::vector<AddressMismatch> mismatches_;
    std::vector<ResponseMismatch> response_mismatches_;
};

} // namespace tb
} // namespace pcie
} // namespace keraunos

#endif // TB_COMMON_H

