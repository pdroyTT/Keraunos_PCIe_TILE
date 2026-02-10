#ifndef TB_TLB_REFERENCE_MODEL_H
#define TB_TLB_REFERENCE_MODEL_H

#include "../include/keraunos_pcie_tlb_common.h"
#include "tb_common.h"
#include <vector>
#include <cstdint>
#include <sc_dt.h>

namespace keraunos {
namespace pcie {
namespace tb {

/**
 * Reference Model for TLB Translation
 * 
 * Implements the specification algorithms for address translation
 * Used as golden model for comparison
 */
class TlbReferenceModel {
public:
    TlbReferenceModel() {}
    virtual ~TlbReferenceModel() {}
    
    // Configure TLB entry
    virtual void configure_entry(uint8_t index, const TlbEntry& entry) = 0;
    virtual TlbEntry get_entry(uint8_t index) const = 0;
    
    // Lookup methods
    virtual bool lookup(uint64_t input_addr, uint64_t& translated_addr,
                       uint32_t& axuser) const = 0;
    
    virtual bool lookup_outbound(uint64_t input_addr, uint64_t& translated_addr,
                                sc_dt::sc_bv<256>& attr) const = 0;
};

/**
 * Reference Model for TLBSysIn0
 */
class TlbSysIn0ReferenceModel : public TlbReferenceModel {
public:
    TlbSysIn0ReferenceModel() : entries_(64) {}
    
    void configure_entry(uint8_t index, const TlbEntry& entry) override {
        if (index < entries_.size()) {
            entries_[index] = entry;
        }
    }
    
    TlbEntry get_entry(uint8_t index) const override {
        if (index < entries_.size()) {
            return entries_[index];
        }
        return TlbEntry();
    }
    
    bool lookup(uint64_t input_addr, uint64_t& translated_addr,
               uint32_t& axuser) const override {
        // Index calculation: [51:14] -> [37:0] -> index [5:0]
        uint8_t index = (input_addr >> 14) & 0x3F;
        
        if (index >= entries_.size() || !entries_[index].valid) {
            translated_addr = INVALID_ADDRESS_DECERR;
            return false;
        }
        
        const TlbEntry& entry = entries_[index];
        
        // Translate address: {TLBSysIn0[index].ADDR[51:14], pa[13:0]}
        translated_addr = (entry.addr & 0xFFFFFFFFFC000ULL) | 
                         (input_addr & 0x3FFF);
        
        // AxUSER: {ATTR[11:4], 2'b0, ATTR[1:0]}
        // Extract bits from attr sc_bv<256>
        // Note: attr[11:4] means bits 11 down to 4, attr[1:0] means bits 1 down to 0
        sc_dt::sc_bv<8> attr_high = entry.attr.range(11, 4);
        sc_dt::sc_bv<2> attr_low = entry.attr.range(1, 0);
        axuser = (attr_high.to_uint() << 4) | attr_low.to_uint();
        
        return true;
    }
    
    bool lookup_outbound(uint64_t input_addr, uint64_t& translated_addr,
                        sc_dt::sc_bv<256>& attr) const override {
        // Not applicable for inbound TLB
        return false;
    }
    
private:
    std::vector<TlbEntry> entries_;
};

/**
 * Reference Model for TLBAppIn0
 */
class TlbAppIn0ReferenceModel : public TlbReferenceModel {
public:
    TlbAppIn0ReferenceModel() : entries_(64) {}
    
    void configure_entry(uint8_t index, const TlbEntry& entry) override {
        if (index < entries_.size()) {
            entries_[index] = entry;
        }
    }
    
    TlbEntry get_entry(uint8_t index) const override {
        if (index < entries_.size()) {
            return entries_[index];
        }
        return TlbEntry();
    }
    
    bool lookup(uint64_t input_addr, uint64_t& translated_addr,
               uint32_t& axuser) const override {
        // Check port: iatu_addr[63:60] should be 0 for BAR0/1
        uint8_t port = (input_addr >> 60) & 0x1;
        if (port != 0) {
            translated_addr = INVALID_ADDRESS_DECERR;
            return false;
        }
        
        // Index calculation: [51:24] -> [27:0] -> index [5:0]
        uint8_t index = (input_addr >> 24) & 0x3F;
        
        if (index >= entries_.size() || !entries_[index].valid) {
            translated_addr = INVALID_ADDRESS_DECERR;
            return false;
        }
        
        const TlbEntry& entry = entries_[index];
        
        // Translate address: {TLBAppIn0[index].ADDR[51:24], pa[23:0]}
        translated_addr = (entry.addr & 0xFFFFFF000000ULL) | 
                         (input_addr & 0xFFFFFF);
        
        // AxUSER: {3'b0, ATTR[4:0], 4'b0}
        sc_dt::sc_bv<5> attr_low = entry.attr.range(4, 0);
        axuser = attr_low.to_uint() << 4;
        
        return true;
    }
    
    bool lookup_outbound(uint64_t input_addr, uint64_t& translated_addr,
                        sc_dt::sc_bv<256>& attr) const override {
        return false;
    }
    
private:
    std::vector<TlbEntry> entries_;
};

/**
 * Reference Model for TLBAppOut0
 */
class TlbAppOut0ReferenceModel : public TlbReferenceModel {
public:
    TlbAppOut0ReferenceModel() : entries_(16) {}
    
    void configure_entry(uint8_t index, const TlbEntry& entry) override {
        if (index < entries_.size()) {
            entries_[index] = entry;
        }
    }
    
    TlbEntry get_entry(uint8_t index) const override {
        if (index < entries_.size()) {
            return entries_[index];
        }
        return TlbEntry();
    }
    
    bool lookup(uint64_t input_addr, uint64_t& translated_addr,
               uint32_t& axuser) const override {
        return false;  // Not applicable
    }
    
    bool lookup_outbound(uint64_t pa, uint64_t& translated_addr,
                        sc_dt::sc_bv<256>& attr) const override {
        // Only process addresses >= 256TB
        if (pa < (1ULL << 48)) {
            translated_addr = INVALID_ADDRESS_DECERR;
            return false;
        }
        
        // Index calculation: [63:44] -> [19:0] -> index [3:0]
        uint8_t index = (pa >> 44) & 0xF;
        
        if (index >= entries_.size() || !entries_[index].valid) {
            translated_addr = INVALID_ADDRESS_DECERR;
            return false;
        }
        
        const TlbEntry& entry = entries_[index];
        
        // Translate address: {TLBAppOut0[index].ADDR[63:44], pa[43:0]}
        translated_addr = (entry.addr & 0xFFFFF00000000000ULL) | 
                         (pa & 0xFFFFFFFFFFFULL);
        
        // Copy attributes
        attr = entry.attr;
        
        return true;
    }
    
private:
    std::vector<TlbEntry> entries_;
};

} // namespace tb
} // namespace pcie
} // namespace keraunos

#endif // TB_TLB_REFERENCE_MODEL_H

