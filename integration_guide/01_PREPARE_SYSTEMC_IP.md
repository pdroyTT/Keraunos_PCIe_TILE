# Step 1: Prepare SystemC IP for VDK Integration

## Overview

This guide covers how to prepare your existing SystemC IP (like TLB modules) for integration into Synopsys Virtualizer VDK.

**Time Required:** 2-8 hours (depending on IP complexity)

---

## Prerequisites Check

Before starting, verify you have:

- [ ] SystemC IP source code (`.h` and `.cpp` files)
- [ ] SystemC 2.3.x installed
- [ ] TLM 2.0 library available
- [ ] C++17 compatible compiler
- [ ] Basic understanding of TLM 2.0

---

## Step 1.1: Review Your Current IP Structure

### Typical SystemC IP Structure

```cpp
// Example: inbound_tlb.h
#ifndef INBOUND_TLB_H
#define INBOUND_TLB_H

#include <systemc>
#include <tlm>

class inbound_tlb : public sc_core::sc_module {
public:
    // Constructor
    SC_HAS_PROCESS(inbound_tlb);
    inbound_tlb(sc_core::sc_module_name name);
    
    // Ports (if any)
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> rst;
    
    // Methods
    void translate_address();
    
private:
    // Internal state
    std::map<uint64_t, uint64_t> translation_table;
};

#endif
```

### What VDK Needs

For Virtualizer integration, your IP must:

1. **Be a proper `sc_module`**
   ```cpp
   class your_ip : public sc_core::sc_module {
       SC_HAS_PROCESS(your_ip);
       // ...
   };
   ```

2. **Have TLM 2.0 sockets** (if memory-mapped)
   ```cpp
   tlm_utils::simple_target_socket<your_ip> target_socket;
   tlm_utils::simple_initiator_socket<your_ip> initiator_socket;
   ```

3. **Implement `b_transport()` for blocking transport**
   ```cpp
   void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
   ```

---

## Step 1.2: Add TLM 2.0 Interface (If Not Present)

### Basic TLM Target Socket

If your IP receives transactions (memory-mapped register):

```cpp
// In header file (inbound_tlb.h)
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

class inbound_tlb : public sc_core::sc_module {
public:
    // Add TLM target socket
    tlm_utils::simple_target_socket<inbound_tlb> target_socket;
    
    SC_HAS_PROCESS(inbound_tlb);
    inbound_tlb(sc_core::sc_module_name name);
    
    // TLM-2 blocking transport method
    virtual void b_transport(tlm::tlm_generic_payload& trans,
                            sc_core::sc_time& delay);
    
private:
    // Your existing implementation
    std::map<uint64_t, uint64_t> tlb_entries;
};
```

### Implementation in Source File

```cpp
// In source file (inbound_tlb.cpp)
#include "inbound_tlb.h"

inbound_tlb::inbound_tlb(sc_core::sc_module_name name)
    : sc_module(name)
    , target_socket("target_socket")
{
    // Register callback
    target_socket.register_b_transport(this, &inbound_tlb::b_transport);
    
    // Your initialization code
}

void inbound_tlb::b_transport(tlm::tlm_generic_payload& trans,
                               sc_core::sc_time& delay)
{
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64 addr = trans.get_address();
    unsigned char* ptr = trans.get_data_ptr();
    unsigned int len = trans.get_data_length();
    
    // Perform translation or register access
    if (cmd == tlm::TLM_READ_COMMAND) {
        // Read from TLB registers
        read_register(addr, ptr, len);
    }
    else if (cmd == tlm::TLM_WRITE_COMMAND) {
        // Write to TLB registers
        write_register(addr, ptr, len);
    }
    
    // Set response status
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    
    // Add timing delay (if needed)
    delay += sc_core::sc_time(10, sc_core::SC_NS);
}
```

### Basic TLM Initiator Socket

If your IP initiates transactions (to memory):

```cpp
// In header file
#include <tlm_utils/simple_initiator_socket.h>

class inbound_tlb : public sc_core::sc_module {
public:
    // Target socket (from devices)
    tlm_utils::simple_target_socket<inbound_tlb> target_socket;
    
    // Initiator socket (to memory)
    tlm_utils::simple_initiator_socket<inbound_tlb> initiator_socket;
    
    // ... rest of class ...
    
private:
    void forward_transaction(uint64_t virt_addr, uint64_t size, 
                            unsigned char* data, bool is_write);
};
```

```cpp
// In source file
void inbound_tlb::forward_transaction(uint64_t virt_addr, uint64_t size,
                                       unsigned char* data, bool is_write)
{
    // Translate address
    uint64_t phys_addr = translate(virt_addr);
    
    // Create TLM transaction
    tlm::tlm_generic_payload trans;
    trans.set_command(is_write ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND);
    trans.set_address(phys_addr);
    trans.set_data_ptr(data);
    trans.set_data_length(size);
    trans.set_streaming_width(size);
    trans.set_byte_enable_ptr(0);
    trans.set_dmi_allowed(false);
    trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    
    // Send transaction
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    initiator_socket->b_transport(trans, delay);
    
    // Check response
    if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
        SC_REPORT_ERROR("TLB", "Transaction failed");
    }
}
```

---

## Step 1.3: Implement Essential Methods

### Memory-Mapped Register Access

```cpp
// In your IP class
class inbound_tlb : public sc_core::sc_module {
private:
    // Register map (example)
    enum Registers {
        CTRL_REG    = 0x00,  // Control register
        STATUS_REG  = 0x04,  // Status register
        ADDR_REG    = 0x08,  // Address register
        DATA_REG    = 0x0C,  // Data register
    };
    
    // Register storage
    uint32_t ctrl_reg;
    uint32_t status_reg;
    
    void read_register(uint64_t addr, unsigned char* data, unsigned int len);
    void write_register(uint64_t addr, unsigned char* data, unsigned int len);
};
```

```cpp
void inbound_tlb::read_register(uint64_t addr, unsigned char* data, 
                                unsigned int len)
{
    uint32_t value = 0;
    
    switch (addr) {
        case CTRL_REG:
            value = ctrl_reg;
            break;
        case STATUS_REG:
            value = status_reg;
            break;
        default:
            SC_REPORT_WARNING("TLB", "Read from invalid register");
            value = 0xDEADBEEF;
            break;
    }
    
    // Copy to data pointer
    memcpy(data, &value, std::min(len, sizeof(value)));
}

void inbound_tlb::write_register(uint64_t addr, unsigned char* data,
                                  unsigned int len)
{
    uint32_t value;
    memcpy(&value, data, std::min(len, sizeof(value)));
    
    switch (addr) {
        case CTRL_REG:
            ctrl_reg = value;
            // Trigger control actions
            break;
        case ADDR_REG:
            // Store address for translation
            break;
        default:
            SC_REPORT_WARNING("TLB", "Write to invalid register");
            break;
    }
}
```

---

## Step 1.4: Add Proper Constructor and Destructor

### Recommended Constructor Pattern

```cpp
inbound_tlb::inbound_tlb(sc_core::sc_module_name name)
    : sc_module(name)
    , target_socket("target_socket")
    , initiator_socket("initiator_socket")
{
    // Register TLM callbacks
    target_socket.register_b_transport(this, &inbound_tlb::b_transport);
    
    // Initialize registers
    ctrl_reg = 0;
    status_reg = 0;
    
    // Initialize translation table
    tlb_entries.clear();
    
    // Register SystemC processes (if any)
    // SC_THREAD(your_thread);
    // SC_METHOD(your_method);
}

inbound_tlb::~inbound_tlb()
{
    // Cleanup if needed
}
```

---

## Step 1.5: Add Debug/Logging Support

### Using sc_report for Logging

```cpp
// In your methods
void inbound_tlb::b_transport(tlm::tlm_generic_payload& trans,
                               sc_core::sc_time& delay)
{
    // Log transaction
    SC_REPORT_INFO("TLB", 
        (std::string("Transaction: ") + 
         (trans.is_read() ? "READ" : "WRITE") +
         " @ 0x" + std::to_string(trans.get_address())).c_str());
    
    // Your implementation
    // ...
}
```

### Conditional Debug Messages

```cpp
class inbound_tlb : public sc_core::sc_module {
private:
    bool debug_enabled;
    
    void debug_log(const std::string& msg) {
        if (debug_enabled) {
            std::cout << sc_core::sc_time_stamp() 
                     << " [TLB] " << msg << std::endl;
        }
    }
};
```

---

## Step 1.6: Remove VDK-Incompatible Features

### Features to Avoid

❌ **DON'T Use:**
- `sc_main()` in your IP (VDK provides this)
- Global variables for configuration
- Direct file I/O without abstraction
- Platform-specific code
- Non-standard port types

✅ **DO Use:**
- Standard TLM sockets
- sc_module hierarchy
- Parameters passed through constructor
- Platform-independent code
- Standard SystemC/TLM types

### Example: Remove sc_main

```cpp
// ❌ DON'T include this in your IP
int sc_main(int argc, char* argv[]) {
    inbound_tlb tlb("tlb");
    sc_core::sc_start();
    return 0;
}
```

VDK generates its own `sc_main`. Your IP should just be a module.

---

## Step 1.7: Create Testbench (Optional but Recommended)

### Standalone Testbench

```cpp
// File: inbound_tlb_tb.cpp
#include "inbound_tlb.h"
#include <tlm_utils/simple_initiator_socket.h>

class tlb_testbench : public sc_core::sc_module {
public:
    tlm_utils::simple_initiator_socket<tlb_testbench> initiator_socket;
    
    tlb_testbench(sc_core::sc_module_name name)
        : sc_module(name)
        , initiator_socket("initiator_socket")
    {
        SC_THREAD(test_process);
    }
    
    void test_process() {
        sc_core::wait(10, sc_core::SC_NS);
        
        // Test write
        uint32_t data = 0x12345678;
        write_reg(0x00, data);
        
        // Test read
        uint32_t read_data;
        read_reg(0x00, read_data);
        
        std::cout << "Read back: 0x" << std::hex << read_data << std::endl;
        
        sc_core::sc_stop();
    }
    
private:
    void write_reg(uint64_t addr, uint32_t data) {
        tlm::tlm_generic_payload trans;
        trans.set_command(tlm::TLM_WRITE_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        trans.set_data_length(4);
        trans.set_streaming_width(4);
        trans.set_byte_enable_ptr(0);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        initiator_socket->b_transport(trans, delay);
    }
    
    void read_reg(uint64_t addr, uint32_t& data) {
        tlm::tlm_generic_payload trans;
        trans.set_command(tlm::TLM_READ_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        trans.set_data_length(4);
        trans.set_streaming_width(4);
        trans.set_byte_enable_ptr(0);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        initiator_socket->b_transport(trans, delay);
    }
};

int sc_main(int argc, char* argv[]) {
    inbound_tlb tlb("tlb");
    tlb_testbench tb("tb");
    
    // Connect
    tb.initiator_socket.bind(tlb.target_socket);
    
    sc_core::sc_start();
    return 0;
}
```

### Build and Test

```bash
# Compile testbench
export SYSTEMC_HOME=/home/pdroy/systemc-local
g++ -std=c++17 -I$SYSTEMC_HOME/include \
    -L$SYSTEMC_HOME/lib-linux64 \
    inbound_tlb.cpp inbound_tlb_tb.cpp \
    -lsystemc -o tlb_test

# Run
./tlb_test
```

---

## Step 1.8: Document Your IP

### Create IP Documentation

Create `inbound_tlb_README.md`:

```markdown
# Inbound TLB IP

## Overview
Translation Lookaside Buffer for inbound device transactions (IOVA → PA)

## Register Map
| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| 0x00   | CTRL | RW     | Control register |
| 0x04   | STAT | RO     | Status register |
| 0x08   | ADDR | RW     | Virtual address |
| 0x0C   | DATA | RW     | Physical address |

## TLM Sockets
- `target_socket`: Receives transactions from devices
- `initiator_socket`: Forwards translated transactions to memory

## Parameters
- `num_entries`: Number of TLB entries (default: 64)
- `page_size`: Page size in bytes (default: 4096)

## Usage Example
```cpp
inbound_tlb tlb("tlb");
// Connect to system bus
bus.target_socket.bind(tlb.target_socket);
tlb.initiator_socket.bind(memory.target_socket);
```
```

---

## Step 1.9: Checklist Before Proceeding

✅ **IP Structure:**
- [ ] Inherits from `sc_core::sc_module`
- [ ] Has `SC_HAS_PROCESS` macro
- [ ] Proper constructor with `sc_module_name`

✅ **TLM Interface:**
- [ ] Has `tlm_utils::simple_target_socket` (if memory-mapped)
- [ ] Has `tlm_utils::simple_initiator_socket` (if initiates transactions)
- [ ] Implements `b_transport()` method
- [ ] Sets proper TLM response status

✅ **Code Quality:**
- [ ] No `sc_main()` in IP files
- [ ] No global variables
- [ ] Proper memory management
- [ ] Error handling implemented

✅ **Testing:**
- [ ] Standalone testbench created
- [ ] Basic functionality verified
- [ ] No compilation warnings

✅ **Documentation:**
- [ ] Register map documented
- [ ] Socket descriptions provided
- [ ] Usage examples included

---

## Common Issues and Solutions

### Issue: Compilation Errors with TLM

**Problem:**
```
error: 'tlm_utils' has not been declared
```

**Solution:**
```cpp
// Add these includes
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
```

### Issue: Socket Registration Fails

**Problem:**
```
Error: socket 'target_socket' is already bound
```

**Solution:**
```cpp
// Register callback in constructor, not at socket creation
inbound_tlb::inbound_tlb(sc_module_name name)
    : sc_module(name)
    , target_socket("target_socket")  // Create socket
{
    // Register callback HERE
    target_socket.register_b_transport(this, &inbound_tlb::b_transport);
}
```

### Issue: Timing Not Working

**Problem:**
Delays are ignored in simulation

**Solution:**
```cpp
void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
    // Update delay, don't reset it
    delay += sc_time(10, SC_NS);  // ADD to existing delay
    
    // Don't do: delay = sc_time(10, SC_NS);  // This resets delay!
}
```

---

## Next Steps

Once your IP passes all checklist items:

→ **Proceed to:** `02_PLATFORM_CREATOR_IMPORT.md`

This will guide you through importing your prepared IP into Platform Creator.

---

## Quick Reference Templates

### Minimal TLM Target Template

```cpp
// minimal_ip.h
#ifndef MINIMAL_IP_H
#define MINIMAL_IP_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

class minimal_ip : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<minimal_ip> target_socket;
    
    SC_HAS_PROCESS(minimal_ip);
    minimal_ip(sc_core::sc_module_name name);
    
    virtual void b_transport(tlm::tlm_generic_payload& trans,
                            sc_core::sc_time& delay);
};

#endif
```

### Build Script Template

```bash
#!/bin/bash
# build_ip.sh

export SYSTEMC_HOME=/home/pdroy/systemc-local

g++ -std=c++17 -g -Wall \
    -I$SYSTEMC_HOME/include \
    -L$SYSTEMC_HOME/lib-linux64 \
    your_ip.cpp \
    -lsystemc \
    -o your_ip_test

echo "Build complete: your_ip_test"
```

---

**You're now ready to import your IP into Platform Creator!**

