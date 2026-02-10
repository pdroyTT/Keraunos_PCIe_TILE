# Makefile for Keraunos PCIE Tile SystemC/SCML Project

# SystemC installation
# Use Virtualizer's SystemC 2.3.4 headers (with Synopsys extensions) for SCML2 compatibility
# Use SystemC 2.3.4 library from /localdev/pdroy/systemc-2.3.4 (matches Virtualizer's expected version)
# Override environment variable if set incorrectly
SYSTEMC_HOME := /localdev/pdroy/systemc-2.3.4
SYSTEMC_INC_VZ = /tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux/common/include
SYSTEMC_INC = $(SYSTEMC_HOME)/include
SYSTEMC_LIB = $(SYSTEMC_HOME)/lib64

# SCML installation
SCML_HOME = /tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03
SCML_INC = $(SCML_HOME)/SLS/linux/common/include
SCML_LIB = $(SCML_HOME)/SLS/linux/common/lib-gcc-9.2-64
TLM_INC = $(SCML_HOME)/SLS/linux/common/include/tlm

# Project directories
SRC_DIR = src
TB_DIR = tb/src
INC_DIR = include
TB_INC_DIR = tb/include

# Compiler and flags
CXX = g++
# Virtualizer's SystemC 2.3.4 requires C++14 or higher
CXXFLAGS = -std=c++14 -Wall -g -O2 -DSC_DISABLE_API_VERSION_CHECK
# Use Virtualizer's SystemC headers (with Synopsys extensions) first
# This ensures SCML2 finds the Synopsys-extended SystemC headers
# Do NOT include SystemC 3.0 path to avoid conflicts - use only Virtualizer's headers
INCLUDES = -I$(SYSTEMC_INC_VZ) -I$(INC_DIR) -I$(TB_INC_DIR) -I$(SCML_INC) -I$(TLM_INC)
LDFLAGS = -L$(SYSTEMC_LIB) -L$(SCML_LIB)
LIBS = -lsystemc -lscml2_testing -lsnps_vp_expat -lpthread -ldl

# Source files
# Exclude stub files from wildcard to avoid duplicates, then add them explicitly
SRC_FILES = $(filter-out $(SRC_DIR)/scml2_%_stub.cpp, $(wildcard $(SRC_DIR)/*.cpp))
TB_FILES = $(wildcard $(TB_DIR)/*.cpp)
# Ensure logging stub is included (explicitly to avoid wildcard duplication)
SRC_FILES += $(SRC_DIR)/scml2_logging_stub.cpp
SRC_FILES += $(SRC_DIR)/scml2_debug_callback_stub.cpp
SRC_FILES += $(SRC_DIR)/scml2_writer_policy_stub.cpp
SRC_FILES += $(SRC_DIR)/scml2_simcontext_stub.cpp
SRC_FILES += $(SRC_DIR)/scml2_event_stub.cpp
SRC_FILES += $(SRC_DIR)/scml2_module_stub.cpp
ALL_SRC = $(SRC_FILES) $(TB_FILES)

# Object files
OBJ_DIR = build/obj
OBJ_FILES = $(ALL_SRC:%.cpp=$(OBJ_DIR)/%.o)

# Executable
EXEC = build/tb_keraunos_pcie

# Default target
all: $(EXEC)

# Create executable
$(EXEC): $(OBJ_FILES) | build
	@echo "Linking $(EXEC)..."
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) $(LDFLAGS) $(LIBS) -o $(EXEC)
	@echo "Build complete: $(EXEC)"

# Compile source files
$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Create directories
build:
	@mkdir -p build

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	@mkdir -p $(OBJ_DIR)/$(TB_DIR)

# Run the testbench
run: $(EXEC)
	@echo "Running testbench..."
	LD_LIBRARY_PATH=$(SYSTEMC_LIB):$(SCML_LIB):$$LD_LIBRARY_PATH ./$(EXEC)

# Clean build artifacts
clean:
	rm -rf build

# Help target
help:
	@echo "Available targets:"
	@echo "  make          - Build the testbench"
	@echo "  make run      - Build and run the testbench"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make help     - Show this help message"

.PHONY: all run clean help

