// Stub implementation for Virtualizer TLM payload tracing
// NOTE: Virtualizer already provides payload_trace class definition in payload_trace.h
// This file should NOT redeclare the class, as it causes a conflict.
// If this file is needed for stub implementations, they should be provided
// without redeclaring the class. Since Virtualizer provides the full implementation,
// this stub file may not be needed and can be excluded from the build.

// DO NOT redeclare payload_trace - Virtualizer already defines it in payload_trace.h
// The class declaration below is commented out to avoid redeclaration conflict:
/*
namespace n_payload_trace {
    class payload_trace {
    public:
        static payload_trace* get_instance();
        static void trace_acquire(void* payload, void* socket);
    };
}
*/

// If stub implementations are needed (they shouldn't be since Virtualizer provides them),
// they would go here. But since Virtualizer already provides the full class and implementations,
// this file can be excluded from compilation or left empty.

