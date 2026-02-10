# Makefile for Manual Test Harness
# Builds and runs manual tests for Keraunos PCIe Tile by compiling sources directly

# Use system compiler with Synopsys SystemC
SNPS_VP_HOME := /tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux
COWARE_CXX_COMPILER := gcc-9.5-64
CXX := g++

# Target executable
TARGET := manual_test_keraunos_pcie

# Model base directory
MODEL_BASEDIR := ../..

# Source files - compile all the model sources directly
SRC_DIR := $(MODEL_BASEDIR)/SystemC/src
SRCS := \
	manual_test_harness.cpp \
	$(SRC_DIR)/keraunos_pcie_tile.cpp \
	$(SRC_DIR)/keraunos_pcie_noc_pcie_switch.cpp \
	$(SRC_DIR)/keraunos_pcie_noc_io_switch.cpp \
	$(SRC_DIR)/keraunos_pcie_smn_io_switch.cpp \
	$(SRC_DIR)/keraunos_pcie_inbound_tlb.cpp \
	$(SRC_DIR)/keraunos_pcie_outbound_tlb.cpp \
	$(SRC_DIR)/keraunos_pcie_msi_relay.cpp \
	$(SRC_DIR)/keraunos_pcie_config_reg.cpp \
	$(SRC_DIR)/keraunos_pcie_clock_reset.cpp \
	$(SRC_DIR)/keraunos_pcie_pll_cgm.cpp \
	$(SRC_DIR)/keraunos_pcie_phy.cpp \
	$(SRC_DIR)/keraunos_pcie_sii.cpp \
	$(SRC_DIR)/keraunos_pcie_external_interfaces.cpp \
	$(SRC_DIR)/scml2_payload_trace_stub.cpp \
	$(SRC_DIR)/scml2_writer_policy_stub.cpp \
	$(SRC_DIR)/scml2_logging_stub.cpp \
	$(SRC_DIR)/scml2_event_stub.cpp \
	$(SRC_DIR)/scml2_module_stub.cpp \
	$(SRC_DIR)/scml2_simcontext_stub.cpp \
	$(SRC_DIR)/scml2_debug_callback_stub.cpp \
	$(SRC_DIR)/scml2_vtable_impls.cpp

# Include paths
SYSTEMC_HOME := /usr/local/systemc-2.3.3
INCLUDES := \
    -I$(SYSTEMC_HOME)/include \
    -I$(SNPS_VP_HOME)/common/include \
    -I$(MODEL_BASEDIR)/SystemC/include \
    -I$(MODEL_BASEDIR)

# Library paths
LIBPATHS := \
    -L$(SYSTEMC_HOME)/lib-linux64 \
    -L$(SNPS_VP_HOME)/common/lib-$(COWARE_CXX_COMPILER)

# Libraries (minimal - use system SystemC, add SCML2 from Synopsys)
LIBS := \
    -lsystemc \
    -lscml2 \
    -lpthread -ldl -lrt

# Compiler flags
CXXFLAGS := \
    -std=c++17 \
    -g \
    -O2 \
    -Wall \
    -Wno-deprecated \
    -fmessage-length=0 \
    -DSC_INCLUDE_DYNAMIC_PROCESSES

# Linker flags
LDFLAGS := \
    -Wl,--export-dynamic \
    -Wl,-rpath,$(SNPS_VP_HOME)/common/lib-$(COWARE_CXX_COMPILER) \
    -Wl,-rpath,$(SNPS_VP_HOME)/common/libso-$(COWARE_CXX_COMPILER) \
    -m64

# Build rules
.PHONY: all clean run help

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "Building manual test harness..."
	@echo "Compiling $(words $(SRCS)) source files..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET) \
		-Wl,--start-group $(LIBPATHS) $(LIBS) -Wl,--end-group $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

run: $(TARGET)
	@echo "Running manual test harness..."
	@echo "========================================"
	./$(TARGET)
	@echo "========================================"

run-trace: $(TARGET)
	@echo "Running manual test harness with VCD tracing..."
	./$(TARGET) --trace
	@echo "VCD trace file created: manual_test.vcd"

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET)
	rm -f *.o
	rm -f *.vcd
	rm -f vpsession
	@echo "Clean complete"

help:
	@echo "Manual Test Harness Makefile"
	@echo "============================"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the manual test executable (default)"
	@echo "  run        - Build and run the tests"
	@echo "  run-trace  - Build and run with VCD waveform tracing"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build test"
	@echo "  make run          # Build and run test"
	@echo "  make run-trace    # Build and run with VCD output"
	@echo "  make clean        # Clean artifacts"
	@echo ""
	@echo "Test Coverage:"
	@echo "  - Reset sequence"
	@echo "  - NOC-N read/write access"
	@echo "  - SMN-N configuration access"
	@echo "  - PCIe inbound data paths (PCIe → TLB → NOC/SMN)"
	@echo "  - PCIe outbound data paths (NOC/SMN → TLB → PCIe)"
	@echo "  - Address routing verification"
	@echo "  - Isolation mode"
	@echo "  - Status register access"
