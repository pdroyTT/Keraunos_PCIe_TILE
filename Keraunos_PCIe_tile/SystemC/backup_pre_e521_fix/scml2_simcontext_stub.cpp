// Stub implementation for SystemC 2.3.4 alloc_new_simcontext() function
// This function was added in SystemC 2.3.4 but doesn't exist in 2.3.3
// We provide a compatibility stub that uses the 2.3.3 API

#include <sysc/kernel/sc_simcontext.h>

namespace sc_core {

// Provide stub implementation for alloc_new_simcontext() which is new in SystemC 2.3.4
// In SystemC 2.3.3, we can use the sc_simcontext constructor directly
sc_simcontext* sc_simcontext::alloc_new_simcontext() {
    // In SystemC 2.3.3, we create a new simcontext using the constructor
    // This is a compatibility layer for 2.3.4 API
    return new sc_simcontext();
}

} // namespace sc_core

