// Stub implementation for SCML2 logging Initializer
// This is needed because the Initializer class is declared but not defined in the library
// The logging functionality may not be fully used in this testbench

#include <scml2_logging/registry.h>

namespace scml2 {
namespace logging {

// Provide stub implementations for Initializer constructor/destructor
// These are minimal implementations that satisfy the linker
Initializer::Initializer() {
    // Empty - logging initialization not required for basic functionality
}

Initializer::~Initializer() {
    // Empty - logging cleanup not required for basic functionality
}

} // namespace logging
} // namespace scml2

