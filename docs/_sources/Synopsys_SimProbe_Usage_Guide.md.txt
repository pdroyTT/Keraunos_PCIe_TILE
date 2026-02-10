# Synopsys Simulation Probes (SimProbe) Usage Guide

## Backdoor Memory Access and Other Use Cases

**Reference:** VP Simulation Probes Python Interface Reference Manual, Version V-2024.03  
**Tool:** Synopsys Virtualizer / Platform Architect  
**Python Version:** 3.10.4

---

## Table of Contents

1. [Overview](#1-overview)
2. [Architecture and Setup](#2-architecture-and-setup)
3. [Backdoor Memory Loading -- Primary Use Case](#3-backdoor-memory-loading----primary-use-case)
   - 3.1 [Using CoreProbe for Backdoor Memory Access](#31-using-coreprobe-for-backdoor-memory-access)
   - 3.2 [Using CoreProbe.load_image() for Image Loading](#32-using-coreprobeload_image-for-image-loading)
   - 3.3 [Using MemoryProbe for Direct Memory Read/Write](#33-using-memoryprobe-for-direct-memory-readwrite)
   - 3.4 [Using InitiatorSocketProbe for Debug Transactions](#34-using-initiatorsocketprobe-for-debug-transactions)
   - 3.5 [Using BasicInitiatorSocketProbe (Utility Wrapper)](#35-using-basicinitiiatorsocketprobe-utility-wrapper)
   - 3.6 [Using IHexReader for Intel HEX File Loading](#36-using-ihexreader-for-intel-hex-file-loading)
   - 3.7 [Using SpLinearMemory for Linear Memory Access](#37-using-splinearmemory-for-linear-memory-access)
   - 3.8 [Integrating SimProbe Scripts with VP Config](#38-integrating-simprobe-scripts-with-vp-config)
4. [Other SimProbe Use Cases](#4-other-simprobe-use-cases)
   - 4.1 [Signal Probing and Fault Injection](#41-signal-probing-and-fault-injection)
   - 4.2 [Clock Manipulation](#42-clock-manipulation)
   - 4.3 [Processor Core Introspection](#43-processor-core-introspection)
   - 4.4 [TLM2 Transaction Observation and Manipulation](#44-tlm2-transaction-observation-and-manipulation)
   - 4.5 [Memory Content Observation](#45-memory-content-observation)
   - 4.6 [Software Function Tracing](#46-software-function-tracing)
   - 4.7 [Register Probing](#47-register-probing)
   - 4.8 [GDA Trace Creation (Data Analysis)](#48-gda-trace-creation-data-analysis)
   - 4.9 [Profiling](#49-profiling)
   - 4.10 [Simulation Control](#410-simulation-control)
5. [Complete Backdoor Memory Loading Examples](#5-complete-backdoor-memory-loading-examples)
6. [Summary of Key APIs for Backdoor Access](#6-summary-of-key-apis-for-backdoor-access)

---

## 1. Overview

Synopsys **Simulation Probes** (commonly referred to as **SimProbe**) is a Python-based instrumentation framework for Synopsys Virtual Prototypes (VPs). It provides a rich API allowing users to interact with, observe, and manipulate simulation models at runtime without modifying the model source code.

The framework is structured into three layers:

| Layer | Module | Purpose |
|-------|--------|---------|
| Setup Interface | `sim_setup` | Configure probes at startup (before simulation begins) |
| Core Interface | `sim` | Runtime probing, observation, and manipulation |
| Utilities | `sim_utils` | Higher-level convenience wrappers and helper classes |

**Key capability:** SimProbe enables **backdoor access** to memory -- the ability to read/write memory contents directly (via debug transactions) without going through the normal bus protocol pipeline. This is critical for:

- Pre-loading firmware/software images before simulation starts
- Initializing memory with test data
- Runtime memory inspection without disturbing simulation timing
- Fault injection by modifying memory contents on-the-fly

---

## 2. Architecture and Setup

### 2.1 Script Types

SimProbe scripts operate in two contexts:

1. **Simulation Startup Script** -- Runs during elaboration, uses `sim_setup` module
2. **SystemC Thread Script** -- Runs during simulation as a SystemC thread, uses `sim` and `sim_utils` modules

### 2.2 Creating a SystemC Thread Script

In the **Simulation Startup Script** (`sim_setup`):

```python
import sim_setup

# Create a standalone SystemC thread script that runs at simulation begin
sim_setup.create_standalone_sc_thread(
    'path/to/my_script.py',          # Path to script (relative to simulation executable)
    ['arg1', 'arg2'],                 # Optional arguments (accessible via sys.argv)
    'my_thread_name'                  # Optional thread name
)
```

### 2.3 Simulation Lifecycle Callbacks

```python
import sim_setup

def on_sim_begin():
    print("Simulation has started -- memories are now accessible")

def on_sim_end():
    print("Simulation ending -- dump final state")

sim_setup.add_begin_of_simulation_callback(on_sim_begin)
sim_setup.add_end_of_simulation_callback(on_sim_end)
```

---

## 3. Backdoor Memory Loading -- Primary Use Case

Backdoor memory access allows reading and writing memory contents via **debug transactions** that bypass the normal TLM2 transport pipeline. This means:

- **No simulation time is consumed** (zero-delay access)
- **No side effects** on bus arbitration, caches, or peripheral behavior
- **Ideal for initialization**, firmware loading, and runtime inspection

There are several mechanisms to achieve backdoor memory loading in SimProbe:

---

### 3.1 Using CoreProbe for Backdoor Memory Access

The `CoreProbe` class provides direct access to a processor core's memory space. This is the most common method for loading data into memory associated with a CPU core.

#### 3.1.1 Constructor

```python
import sim

# Attach to a processor core by its hierarchical name
core = sim.CoreProbe('top.cpu0')
```

#### 3.1.2 Writing Data to Core Memory (Backdoor)

**`set_memory_value()`** performs a debug write access (backdoor) to the core's memory.

```python
import sim

core = sim.CoreProbe('top.cpu0')

# Write a 4-byte value to address 0x2000_0000
data = bytearray([0xDE, 0xAD, 0xBE, 0xEF])
num_units = 4           # Number of MAUs (Minimum Addressable Units) to write
bytes_written = core.set_memory_value(0x20000000, data, num_units)
print(f"Wrote {bytes_written} MAUs to memory")
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `int` | Target memory address |
| `buf` | `bytearray` | Data to write |
| `num_units` | `int` | Number of MAUs to write |
| `unit_size` | `int` (optional) | Chunk size for access (in MAUs) |
| `mem_space` | `str` (optional) | Memory space name (use `get_memory_space_names()` to list) |

#### 3.1.3 Reading Data from Core Memory (Backdoor)

**`get_memory_value()`** performs a debug read access (backdoor).

```python
# Read 16 bytes from address 0x2000_0000
data = core.get_memory_value(0x20000000, 16)
print(f"Memory content: {data.hex()}")
```

#### 3.1.4 Software Data View Memory Access

For architectures with MMUs or memory translation, use the software data view methods:

```python
# Read via software data view (uses virtual addresses)
data = core.get_software_data_view_memory_value(0x80000000, 4)

# Write via software data view
core.set_software_data_view_memory_value(0x80000000, bytearray([0x01, 0x02, 0x03, 0x04]), 4)
```

#### 3.1.5 Querying Available Memory Spaces

```python
spaces = core.get_memory_space_names()
print(f"Available memory spaces: {spaces}")
# e.g., ['default', 'physical', 'virtual', 'secure', 'non-secure']
```

#### 3.1.6 Bulk Data Loading Example (Binary File to Memory)

```python
import sim

def load_binary_to_memory(core_name, file_path, base_address):
    """Load a raw binary file into core memory via backdoor access."""
    core = sim.CoreProbe(core_name)

    with open(file_path, 'rb') as f:
        data = bytearray(f.read())

    chunk_size = 4096  # Write in 4KB chunks for efficiency
    offset = 0

    while offset < len(data):
        chunk = data[offset:offset + chunk_size]
        num_units = len(chunk)
        core.set_memory_value(base_address + offset, chunk, num_units)
        offset += chunk_size

    sim.print_message(f"Loaded {len(data)} bytes from {file_path} to 0x{base_address:08X}")

# Usage
load_binary_to_memory('top.cpu0', 'firmware.bin', 0x20000000)
```

---

### 3.2 Using CoreProbe.load_image() for Image Loading

The **`load_image()`** method is a high-level API that loads complete software images (ELF, Intel HEX, S-Record, etc.) into the core's memory. This is typically the primary mechanism for firmware loading.

```python
import sim

core = sim.CoreProbe('top.cpu0')

# Load an ELF image with symbol/debug info
success = core.load_image(
    'firmware/application.elf',   # Path relative to simulation executable
    offset=0,                      # Address offset (added to all symbol addresses)
    debug_info=True,               # Load symbol information
    format='elf'                   # File format (auto-detected if omitted)
)

if success:
    sim.print_message("Image loaded successfully")
```

**Supported formats** (auto-detected by file extension/header):

| Format | Description |
|--------|-------------|
| ELF | Executable and Linkable Format |
| Intel HEX | `.hex` / `.ihex` files |
| S-Record | Motorola S-Record (`.srec`) |
| Binary | Raw binary |

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | `str` | (required) | Path to the image file |
| `offset` | `int` | `0` | Address offset for all symbols |
| `debug_info` | `bool` | `True` | Whether to load symbol/debug info |
| `format` | `str` | (auto) | Image file format |

> **Note:** Loading a software image involves reading a large file. Consider performance implications when calling this at runtime.

---

### 3.3 Using MemoryProbe for Direct Memory Read/Write

The `MemoryProbe` class provides direct backdoor access to **SCML2 memory objects** (or objects implementing the `MemoryVisibilityAPI` IPT) by their SystemC hierarchical name.

#### 3.3.1 Constructor

```python
import sim

# Create a MemoryProbe for a 4-byte region of a memory object
mem = sim.MemoryProbe(
    'top.memory_subsystem.sram',  # Hierarchical name of the SCML2 memory
    size=4,                        # Size in bytes: 1, 2, 4, or 8
    offset=0                       # Byte offset from start of memory
)
```

#### 3.3.2 Backdoor Read (Debug Read)

```python
# Read value -- no behavior side effects (debug read)
value = mem.get_value()
print(f"Memory value: 0x{value:08X}")

# Read as signed integer
signed_value = mem.get_value(as_signed=True)
```

#### 3.3.3 Backdoor Write (Debug Write)

```python
# Write value -- no behavior side effects (debug write)
mem.set_value(0xDEADBEEF)
```

#### 3.3.4 Clamping Memory Values

Clamping forces a memory location to return a fixed value on all reads, while backing up actual writes. This is useful for fault injection.

```python
# Clamp all bits to current value
mem.set_clamp()

# Clamp with a specific value
mem.set_clamp_with_value(0x12345678)

# Clamp with a specific value and mask (only clamp bits set to 1 in mask)
mem.set_clamp_with_value(0x12345678, mask=0xFF00FF00)

# Release the clamp -- memory returns to last written value
mem.release_clamp()
```

#### 3.3.5 Triggering Read/Write Behavior

Unlike pure debug access, these methods invoke the memory's registered SCML2 callbacks:

```python
# Trigger read behavior (invokes SCML2 read callback)
value = mem.call_read_behavior()

# Trigger write behavior with a specific value
mem.call_write_behavior_with_value(0xCAFEBABE)
```

#### 3.3.6 Iterating Over a Memory Region

```python
import sim

def dump_memory_region(mem_name, start_offset, num_words):
    """Dump a region of memory word-by-word via backdoor access."""
    for i in range(num_words):
        probe = sim.MemoryProbe(mem_name, size=4, offset=start_offset + i * 4)
        val = probe.get_value()
        print(f"  [0x{start_offset + i*4:08X}] = 0x{val:08X}")

dump_memory_region('top.sram', 0x0000, 16)
```

---

### 3.4 Using InitiatorSocketProbe for Debug Transactions

The `InitiatorSocketProbe` attaches to a TLM2/AMBA-PV initiator socket and allows performing both debug and non-debug bus transactions. This is useful when you need to access memory through the actual bus interconnect (e.g., to target address-decoded peripherals).

#### 3.4.1 Debug Write (Backdoor via Bus)

```python
import sim

# Attach to an initiator socket
isock = sim.InitiatorSocketProbe('top.bus_master.initiator_socket')

# Perform a debug write (backdoor, no timing)
data = bytearray([0x01, 0x02, 0x03, 0x04])
bytes_written = isock.debug_write(
    addr=0x40000000,   # Target address
    data_len=4,         # Number of bytes
    buf=data            # Data buffer
)
```

#### 3.4.2 Debug Read (Backdoor via Bus)

```python
# Perform a debug read (backdoor, no timing)
buf = bytearray(4)
bytes_read = isock.debug_read(
    addr=0x40000000,
    data_len=4,
    buf=buf
)
print(f"Read data: {buf.hex()}")
```

#### 3.4.3 Non-Debug (Timed) Transactions

These consume simulation time and go through the full transport pipeline:

```python
# Timed write transaction (blocks the SystemC thread)
buf = bytearray([0xAA, 0xBB, 0xCC, 0xDD])
end_time = isock.write(
    time=(0, 'ns'),       # Timing annotation
    addr=0x40000000,
    data_len=4,
    buf=buf
)

# Timed read transaction
buf = bytearray(4)
end_time = isock.read(
    time=(0, 'ns'),
    addr=0x40000000,
    data_len=4,
    buf=buf
)
```

---

### 3.5 Using BasicInitiatorSocketProbe (Utility Wrapper)

The `BasicInitiatorSocketProbe` from `sim_utils` provides convenience methods that abstract away bytearray handling:

```python
from sim_utils import BasicInitiatorSocketProbe

probe = BasicInitiatorSocketProbe('top.bus_master.initiator_socket')

# Fixed-size debug reads (return integers by default)
val8  = probe.debug_read1(0x40000000)   # Read 1 byte
val16 = probe.debug_read2(0x40000000)   # Read 2 bytes
val32 = probe.debug_read4(0x40000000)   # Read 4 bytes
val64 = probe.debug_read8(0x40000000)   # Read 8 bytes

# Fixed-size debug writes
probe.debug_write1(0x40000000, 0xFF)            # Write 1 byte
probe.debug_write2(0x40000000, 0xBEEF)          # Write 2 bytes
probe.debug_write4(0x40000000, 0xDEADBEEF)      # Write 4 bytes
probe.debug_write8(0x40000000, 0xCAFEBABEDEADBEEF)  # Write 8 bytes

# Memory fill -- fill a region with a pattern
probe.mem_fill(0x20000000, 0x1000, bytearray([0x00]))   # Zero-fill 4KB

# Memory copy -- copy data between addresses
probe.mem_copy(src_addr=0x20000000, dst_addr=0x30000000, length=0x1000)

# Read a null-terminated string from memory
text = probe.get_string(0x20001000)
print(f"String at memory: {text}")
```

#### 3.5.1 Loading a Binary File via InitiatorSocketProbe

```python
from sim_utils import BasicInitiatorSocketProbe

def load_binary_via_bus(socket_name, file_path, base_address):
    """Load a binary file into memory via debug transactions on the bus."""
    probe = BasicInitiatorSocketProbe(socket_name)

    with open(file_path, 'rb') as f:
        data = bytearray(f.read())

    chunk_size = 256
    offset = 0
    while offset < len(data):
        chunk = data[offset:offset + chunk_size]
        probe.debug_write(base_address + offset, len(chunk), chunk)
        offset += chunk_size

    print(f"Loaded {len(data)} bytes to 0x{base_address:08X} via {socket_name}")

# Usage
load_binary_via_bus('top.bus.init_socket', 'test_data.bin', 0x80000000)
```

---

### 3.6 Using IHexReader for Intel HEX File Loading

The `sim_utils` library includes `IHexReader` for parsing and loading Intel HEX format files:

```python
from sim_utils import IHexReader

# Create an IHexReader for a .hex file
reader = IHexReader('firmware.hex')

# Read the records from the file
reader.read()

# The reader provides parsed data records that can be
# iterated and written to memory via any of the above probes
```

---

### 3.7 Using SpLinearMemory for Linear Memory Access

`SpLinearMemory` provides a linear memory abstraction that can be used as a staging buffer:

```python
from sim_utils import SpLinearMemory

# Create a linear memory of 4KB
linear_mem = SpLinearMemory(size=4096)

# Write data into linear memory
linear_mem.write(offset=0, data=bytearray([0x01, 0x02, 0x03, 0x04]))

# Read data from linear memory
data = linear_mem.read(offset=0, length=4)
```

---

### 3.8 Integrating SimProbe Scripts with VP Config

SimProbe scripts are **invoked from the VP Config (vpconfig) initialization infrastructure**. The VP Config editor provides multiple script entry points at which SimProbe functionality can be registered and executed. Understanding these entry points is essential for properly orchestrating backdoor memory loading and other SimProbe operations within the simulation lifecycle.

#### 3.8.1 VP Config Script Entry Points Overview

The VP Config editor provides **three distinct script entry points**, each serving a different purpose in the simulation lifecycle:

| Entry Point | Executed By | Timing | Available API | Typical Use |
|---|---|---|---|---|
| **Simulation Startup Scripts** | Simulation process | Before `sc_main()` (earliest) | `sim_setup` only | Register threads, callbacks, set up environment |
| **Simulation Script Callbacks** | Simulation interpreter | At lifecycle events (Begin/End of Sim, etc.) | `sim` (full API) | Memory initialization, state setup |
| **Standalone SystemC Thread Scripts** | Separate SystemC threads | After simulation begins | `sim` and `sim_utils` (full API) | Runtime probing, monitoring, fault injection |

The following diagram illustrates the invocation flow:

```
VP Config (vpconfig)
  |
  |-- Simulation Startup Script: startup.py        [uses sim_setup]
  |     |-- Creates threads via create_standalone_sc_thread()
  |     |-- Registers callbacks via add_begin_of_simulation_callback()
  |     '-- Sets up environment variables and paths
  |
  |-- Simulation Script Callbacks:
  |     |-- Begin of Simulation scripts             [uses sim — full API]
  |     |-- End of Simulation scripts               [uses sim — full API]
  |     '-- Advanced: about_to_suspend, checkpointing, restore
  |
  '-- Standalone SystemC Thread Scripts table:
        |-- monitor_script.py                       [uses sim, sim_utils]
        |-- backdoor_loader.py                      [uses sim, sim_utils]
        '-- fault_injector.py                       [uses sim, sim_utils]
```

#### 3.8.2 Simulation Startup Scripts (via `sim_setup` Module)

The **Simulation Startup Script** is the earliest entry point. It is specified in the VP Config **Overview tab** under the "Simulation Startup Script" field (under the Details menu). It is executed by the simulation process **before `sc_main()` is called** and must complete before simulation creation can continue.

From this script, you import the `sim_setup` module and can:

- Create standalone SystemC thread scripts (which later use the full `sim` and `sim_utils` APIs)
- Register lifecycle callbacks (begin-of-simulation, end-of-simulation, checkpointing, etc.)
- Set up file system paths and environment variables
- Initialize simulation-wide state

**Example: Simulation Startup Script (`startup.py`) specified in VP Config:**

```python
import sim_setup
import os

# Resolve script paths relative to this startup script's directory
script_dir = os.path.dirname(os.path.abspath(__file__))

# Register a SimProbe thread script to run at simulation begin
backdoor_script = os.path.join(script_dir, 'backdoor_loader.py')
sim_setup.create_standalone_sc_thread(
    backdoor_script,
    ['firmware/app.elf', '0x20000000'],   # Arguments via sys.argv
    'backdoor_loader'                      # Thread name
)

# Register a begin-of-simulation callback for quick initialization
def on_begin_of_sim():
    print("Simulation starting — all model hierarchy is now constructed")

sim_setup.add_begin_of_simulation_callback(on_begin_of_sim)

# Register an end-of-simulation callback for cleanup
def on_end_of_sim():
    print("Simulation ending — dumping final state")

sim_setup.add_end_of_simulation_callback(on_end_of_sim)
```

**Key `sim_setup` methods available in the Startup Script:**

| Method | Description |
|---|---|
| `create_standalone_sc_thread(path, [args], [name])` | Register a Python script to run as a standalone SystemC thread |
| `add_begin_of_simulation_callback(cb)` | Register a callback invoked after SystemC elaboration |
| `add_end_of_simulation_callback(cb)` | Register a callback invoked at simulation shutdown |
| `add_about_to_suspend_callback(cb)` | Register a callback invoked before simulation suspends |
| `add_begin_of_checkpointing_callback(cb)` | Register a callback invoked before checkpointing |
| `add_end_of_checkpointing_callback(cb)` | Register a callback invoked after checkpointing |
| `add_end_of_restore_callback(cb)` | Register a callback invoked after restoring a checkpoint |

#### 3.8.3 Simulation Script Callbacks (Begin/End of Simulation)

The VP Config editor provides dedicated **tables** where you can directly list Python scripts that will be executed at specific lifecycle points. These scripts run in the main simulation interpreter and have access to the full `sim` API.

The most commonly used callbacks are:

- **Begin of Simulation** -- invoked after the SystemC hierarchy is constructed and elaborated
- **End of Simulation** -- invoked at simulation shutdown

Additional callbacks are available under the **Advanced** section of the VP Config editor:

- **About to Suspend** -- invoked before simulation suspends
- **Begin/End of Checkpointing** -- invoked around checkpoint operations
- **End of Restore** -- invoked after restoring a checkpoint

**Example: Begin-of-Simulation callback script for backdoor memory initialization (`init_memory.py`):**

```python
import sim

# This script is listed in the VP Config "Begin of Simulation" callback table.
# At this point, the full SystemC hierarchy is elaborated and sim API is available.

core = sim.CoreProbe('top.cpu0')

# Load firmware via backdoor
success = core.load_image('firmware/app.elf', offset=0, debug_info=True, format='elf')
if success:
    sim.print_message("Firmware loaded successfully at begin-of-simulation")

# Initialize a shared memory region with test data
mem = sim.MemoryProbe('top.shared_sram', size=4, offset=0)
mem.set_value(0x00000000)  # Clear first word

sim.print_message("Memory initialization complete")
```

#### 3.8.4 Standalone SystemC Thread Scripts in VP Config

The VP Config editor includes a **Standalone SystemC Threads** table where you can directly list Python scripts to be executed as independent SystemC threads. Each script runs in its own Python sub-interpreter context and SystemC thread, with full access to `sim` and `sim_utils` libraries.

Scripts listed in this table are automatically created as SystemC thread scripts at simulation start -- there is no need to explicitly call `sim_setup.create_standalone_sc_thread()` for them.

> **Note:** Variables, methods, and imported libraries are local to each standalone SystemC thread script and cannot be accessed from other scripts. To share functionality between thread scripts, use the `ThreadInterface` class or inter-thread message passing via `sim.post_message()`.

#### 3.8.5 Specifying Scripts from the Command Line

When launching the simulation from a command line (without the Virtualizer Studio IDE), the Simulation Startup Script can be specified via the `--snps_startup_script` parameter:

```bash
# Using vpsession
vpsession sim -snps_startup_script startup.py

# The script_path can be absolute or relative to the simulation directory
vpsession sim -snps_startup_script /path/to/startup.py
```

#### 3.8.6 Recommended Architecture for Backdoor Memory Loading via VP Config

The recommended architecture for using SimProbe to load data via backdoor access from VP Config is a **layered approach** that separates concerns across the different entry points:

```
VP Config
  |
  |-- Simulation Startup Script: startup.py          [sim_setup]
  |     |-- Creates thread: backdoor_loader.py
  |     |-- Creates thread: monitor_script.py
  |     '-- Registers callback: on_begin_of_sim()
  |
  |-- Begin of Simulation Callback Scripts:
  |     '-- init_memory.py                            [sim — backdoor writes]
  |
  |-- End of Simulation Callback Scripts:
  |     '-- dump_state.py                             [sim — state dump]
  |
  '-- Standalone SystemC Thread Scripts:
        |-- backdoor_loader.py                        [sim, sim_utils — firmware load]
        '-- monitor_script.py                         [sim, sim_utils — runtime monitoring]
```

**Separation of concerns:**

| Layer | Responsibility | API |
|---|---|---|
| Startup Script | Orchestration: register threads and callbacks | `sim_setup` |
| Begin-of-Sim Callbacks | One-time initialization: load firmware, set initial register values | `sim` |
| SystemC Thread Scripts | Runtime operations: monitoring, fault injection, test stimuli | `sim`, `sim_utils` |
| End-of-Sim Callbacks | Cleanup: dump final state, close log files | `sim` |

#### 3.8.7 Key Constraint: `sim_setup` vs `sim` Module

A critical constraint to understand is that the **Simulation Startup Script** (which runs before `sc_main()`) can **only** use the `sim_setup` module. It **cannot** directly use runtime probe classes such as `sim.CoreProbe`, `sim.MemoryProbe`, `sim.InitiatorSocketProbe`, or any `sim_utils` utilities.

Those runtime probes are only available:

1. Inside **SystemC Thread Scripts** (created via `sim_setup.create_standalone_sc_thread()` or listed in the VP Config Standalone SystemC Threads table)
2. Inside **begin-of-simulation callback** scripts (registered via `sim_setup.add_begin_of_simulation_callback()` or listed in the VP Config Begin of Simulation callback table)
3. Inside any other **lifecycle callback** scripts that execute during simulation runtime

**The typical pattern is therefore:**

1. The VP Config Simulation Startup Script uses `sim_setup` to *register* SimProbe scripts and callbacks
2. The actual backdoor memory operations happen when those registered scripts execute at simulation begin or during runtime

```python
# WRONG — will fail in a Simulation Startup Script:
import sim
core = sim.CoreProbe('top.cpu0')  # ERROR: sim module not available here

# CORRECT — register a thread that will use sim at runtime:
import sim_setup
import os
script_dir = os.path.dirname(os.path.abspath(__file__))
sim_setup.create_standalone_sc_thread(
    os.path.join(script_dir, 'load_firmware.py')
)
```

#### 3.8.8 Complete Example: VP Config-Driven Backdoor Loading

**File 1: `startup.py`** (Simulation Startup Script -- specified in VP Config Overview tab)

```python
"""
Simulation Startup Script: Registers all SimProbe threads and callbacks.
Specified in VP Config > Overview > Simulation Startup Script field.
Uses sim_setup module only.
"""
import sim_setup
import os

script_dir = os.path.dirname(os.path.abspath(__file__))

# 1. Register a begin-of-simulation callback for quick memory init
def init_shared_memory():
    import sim
    # Initialize shared memory region to zero
    for offset in range(0, 0x1000, 4):
        mem = sim.MemoryProbe('top.shared_sram', size=4, offset=offset)
        mem.set_value(0x00000000)
    sim.print_message("Shared memory zeroed at begin-of-simulation")

sim_setup.add_begin_of_simulation_callback(init_shared_memory)

# 2. Register the firmware loader as a standalone SystemC thread
firmware_loader = os.path.join(script_dir, 'load_firmware.py')
sim_setup.create_standalone_sc_thread(
    firmware_loader,
    ['firmware/app.elf', '0x20000000'],
    'firmware_loader'
)

# 3. Register a monitoring thread
monitor_script = os.path.join(script_dir, 'bus_monitor.py')
sim_setup.create_standalone_sc_thread(
    monitor_script,
    [],
    'bus_monitor'
)

# 4. Register an end-of-simulation callback for state dump
def dump_final_state():
    import sim
    core = sim.CoreProbe('top.cpu0')
    pc = core.get_core_register_value('PC')
    sim.print_message(f"Final PC: 0x{pc:08X}")
    sim.print_message(f"Instructions executed: {core.number_of_executed_instructions}")

sim_setup.add_end_of_simulation_callback(dump_final_state)
```

**File 2: `load_firmware.py`** (SystemC Thread Script -- registered by startup.py)

```python
"""
SystemC Thread Script: Loads firmware into memory via backdoor access.
Registered by startup.py via sim_setup.create_standalone_sc_thread().
Uses sim and sim_utils modules.
"""
import sim
import sys
from sim_utils import INFO_MSG, FATAL_MSG

def main():
    # Parse arguments passed from startup.py
    firmware_path = sys.argv[1] if len(sys.argv) > 1 else 'firmware/app.elf'
    base_addr_str = sys.argv[2] if len(sys.argv) > 2 else '0x20000000'
    base_addr = int(base_addr_str, 16)

    INFO_MSG(f"Loading firmware: {firmware_path} at 0x{base_addr:08X}")

    try:
        core = sim.CoreProbe('top.cpu0')

        # Load ELF image with symbol info via backdoor
        success = core.load_image(firmware_path, offset=base_addr, debug_info=True)

        if success:
            INFO_MSG("Firmware loaded successfully")
            # Verify reset vector
            reset_vec = core.get_memory_value(base_addr, 4)
            INFO_MSG(f"Reset vector at 0x{base_addr:08X}: {reset_vec.hex()}")
        else:
            FATAL_MSG("Failed to load firmware image")

    except RuntimeError as e:
        FATAL_MSG(f"Firmware load error: {e}")

main()
```

**File 3: `bus_monitor.py`** (SystemC Thread Script -- registered by startup.py)

```python
"""
SystemC Thread Script: Monitors bus traffic during simulation.
Registered by startup.py via sim_setup.create_standalone_sc_thread().
"""
import sim
from sim_utils import INFO_MSG

# Set up bus transaction observation
trace = sim.SocketTraceProbe('top.bus.target_socket')

def on_transaction_begin(observer, args):
    INFO_MSG(f"Bus transaction: addr=0x{args.address:08X}, "
             f"type={args.type}, size={args.data_length}")

observer = trace.create_observer(begin_cb=on_transaction_begin)

INFO_MSG("Bus monitor armed — observing transactions")

# Keep the thread alive for the duration of the simulation
sim.wait_until((1000, 's'))
```

#### 3.8.9 Script Path Resolution in VP Config

When specifying script paths in the VP Config, paths can be expressed in several ways:

| Path Type | Syntax | Description |
|---|---|---|
| Relative to vpconfig dir | `${vpconfigDir}/my_script.py` | Resolved relative to the VP Config file location |
| Relative to script library | `${LIBRARY_ID}/scripts/my_script.py` | Resolved via the script library manager |
| Absolute path | `/path/to/my_script.py` | Absolute filesystem path |
| Relative to sim executable | `my_script.py` | Resolved relative to the simulation executable directory |

> **Note:** The working directory for Simulation Startup Scripts and SystemC thread scripts is the directory of the simulation executable, **not** the VP Config directory. Use `os.path.dirname(os.path.abspath(__file__))` to resolve paths relative to the script's own location.

#### 3.8.10 Script Library Integration

SimProbe scripts can also be packaged as part of **script libraries** and delivered alongside IP libraries. A script library is defined by an `*.iplib` file:

```json
{
    "format": 1,
    "ip_libraries": [{
        "name": "MY_SCRIPT_LIBRARY",
        "package_allowed": true,
        "scripts_dir": "scripts"
    }],
    "examples": [{
        "script_type": "sim_probe",
        "is_template": true,
        "description": "Backdoor memory loader template",
        "categories": ["memory", "initialization"],
        "files": ["templates/backdoor_loader_template.py"]
    }]
}
```

Valid `script_type` values for the iplib registration:

| Script Type | Description |
|---|---|
| `sim_probe` | Standalone SystemC thread script |
| `startup_script` | Simulation Startup Script |
| `startup_begin_sim_script` | Begin-of-simulation callback script |
| `startup_end_sim_script` | End-of-simulation callback script |
| `vpx_script` | VP Explorer script |
| `vdkc_script` | VDK Debug CLI script |

Scripts in libraries placed inside the VDK project's `script_lib` directory are automatically added to the Python path.

---

## 4. Other SimProbe Use Cases

Beyond backdoor memory loading, SimProbe provides a comprehensive set of capabilities for simulation introspection and control.

---

### 4.1 Signal Probing and Fault Injection

SimProbe can attach to SystemC signals of various data types and read, write, clamp, or observe their values.

**Available probe types:** `BoolProbe`, `IntProbe`, `UintProbe`, `Uint64Probe`, `FloatProbe`, `DoubleProbe`, `LogicProbe`, `LV<N>Probe`, `ScInt<N>Probe`, `ScUint<N>Probe`, `ScBigInt<N>Probe`, `ScBigUint<N>Probe`, `CharProbe`, `ShortProbe`, `TimeProbe`, and more.

```python
import sim

# Read a signal value
sig = sim.UintProbe('top.subsystem.my_signal')
current_val = sig.get_value()
print(f"Signal value: {current_val}")

# Write a signal value
sig.set_value(42)

# Clamp a signal to a fixed value (fault injection)
sig.set_clamp_value(0)            # Force signal to 0 indefinitely
sig.set_clamp_value_for(1, (10, 'us'))  # Clamp to 1 for 10us then release

# Wait for a signal condition (blocks the SystemC thread)
sig.wait_for_value(1)             # Wait until signal equals 1
sig.wait_for_any_value_change()   # Wait for any change

# Register a callback on signal change
def on_signal_change(probe):
    print(f"Signal changed to: {probe.get_value()}")
    probe.callback_on_any_value_change()  # Re-arm (one-shot)

sig.set_callback(on_signal_change)
sig.callback_on_any_value_change()

# Release clamp
sig.release_clamp()
```

---

### 4.2 Clock Manipulation

The `ClockProbe` allows reading, modifying, and clamping SCML clock properties.

```python
import sim

clk = sim.ClockProbe('top.clk_gen.main_clock')

# Read clock properties
period = clk.get_period()          # Returns (amount, unit) tuple
enabled = clk.get_enabled()
running = clk.get_running()
print(f"Clock period: {period}, enabled: {enabled}, running: {running}")

# Modify clock period
clk.set_period((10, 'ns'))         # Set to 10ns period (100MHz)

# Disable/enable clock
clk.set_enabled(False)             # Gate the clock
clk.set_enabled(True)              # Ungate

# Clamp clock period (prevent SW from changing it)
clk.set_clamp_period((20, 'ns'))   # Force 50MHz
clk.set_clamp_enabled(True)        # Force clock enabled

# Timed clamp -- clamp for a duration, then release
clk.set_clamp_period_for((5, 'ns'), (100, 'us'))  # 200MHz for 100us

# Wait for clock property changes
clk.wait_for_period_change((10, 'ns'))

# Release all clamps
clk.release_clamp()
```

---

### 4.3 Processor Core Introspection

The `CoreProbe` provides deep introspection into processor cores:

```python
import sim

core = sim.CoreProbe('top.cpu0')

# -- Core Properties --
print(f"Architecture: {core.architecture_name}")
print(f"Architecture Family: {core.architecture_family_name}")
print(f"Instance: {core.instance_name}")
print(f"Instructions executed: {core.number_of_executed_instructions}")
print(f"Cycles executed: {core.number_of_executed_cycles}")
print(f"Current context: {core.current_context}")

# -- Register Access --
regs = core.core_registers  # List all register names
print(f"Available registers: {regs}")

pc_val = core.get_core_register_value('PC')
print(f"Program Counter: 0x{pc_val:08X}")

core.set_core_register_value('R0', 0x12345678)  # Set register value

# -- Symbol Lookup --
sym = core.find_symbol_by_name('main', 'function')
print(f"main() at address: 0x{sym.address:08X}, size: {sym.size}")

# -- Disassembly --
mnemonic, opcode = core.get_disassembly_info(0x20000000)
print(f"Instruction at 0x20000000: {mnemonic} ({opcode})")

# -- Debug Info (DWARF) --
dbg = core.get_debug_info('')  # Empty string = no OS context
var_info = dbg.find_variable('global_counter')
print(f"Variable '{var_info.name}' type: {var_info.type_info.name}")
val = var_info.get_typed_value()
print(f"Value: {val}")
```

---

### 4.4 TLM2 Transaction Observation and Manipulation

#### 4.4.1 Observing Transactions (TargetSocketProbe / SocketTraceProbe)

```python
import sim

# Observe all transactions on a TLM2 socket
trace = sim.SocketTraceProbe('top.bus.target_socket')

def on_begin(observer, args):
    fields = args.list()
    print(f"Transaction BEGIN: type={args.type}, time={args.time}")
    for name, val in fields:
        print(f"  {name} = {val}")

def on_end(observer, args):
    print(f"Transaction END: type={args.type}")

observer = trace.create_observer(
    begin_cb=on_begin,
    end_cb=on_end
)
```

#### 4.4.2 Transaction Manipulation (TargetSocketProbe)

The `TargetSocketProbe` allows not just observing but also modifying transaction arguments (address, data, response status) in-flight.

---

### 4.5 Memory Content Observation

The `MemoryContentObserver` triggers callbacks on memory accesses to specific address ranges:

```python
import sim

def on_memory_access(observer, args):
    val = args.get_value()
    prev = args.get_before_access_value()
    print(f"Memory {args.access_kind} at index {args.index}: "
          f"0x{prev:08X} -> 0x{val:08X}")

# Observe writes to indices 0-255 of an SCML2 memory
obs = sim.MemoryContentObserver(
    'top.sram.memory',     # Memory object name
    start=0,                # Start index
    end=255,                # End index
    cb=on_memory_access,
    access_kind='write'     # 'read', 'write', or 'any'
)
```

---

### 4.6 Software Function Tracing

#### 4.6.1 Function Entry/Exit Observer

```python
import sim

core = sim.CoreProbe('top.cpu0')

def on_function(observer, args):
    change = args.functionChange  # 'Call' or 'Return'
    func_name = args.symbol.name
    context = args.context
    print(f"[{sim.get_time()}] {change}: {func_name} (context: {context})")

func_obs = core.create_enter_function_observer(on_function)
```

#### 4.6.2 Instruction Execution Observer

```python
import sim

core = sim.CoreProbe('top.cpu0')

def on_instruction(observer, args):
    print(f"Executing instruction at 0x{args.address:08X}")

# Trigger callback when PC enters the address range [0x2000_0000, 0x2000_0100]
instr_obs = core.create_about_to_execute_instruction_observer(
    cb=on_instruction,
    first=0x20000000,
    last=0x20000100
)
```

#### 4.6.3 Register Access Observer

```python
import sim

core = sim.CoreProbe('top.cpu0')

def on_reg_access(observer, args):
    print(f"Register {args.name} {args.access_kind}: "
          f"0x{args.prev_value:08X} -> 0x{args.value:08X} "
          f"(PC=0x{args.hit_pc:08X})")

reg_obs = core.create_core_register_observer(
    cb=on_reg_access,
    registers=['R0', 'R1', 'SP', 'LR'],
    access_kind='write'        # 'read', 'write', or 'any'
)
```

#### 4.6.4 Software Memory Access Observer

```python
import sim

core = sim.CoreProbe('top.cpu0')

def on_sw_mem_access(observer, args):
    print(f"SW memory {args.access_kind} at sw_addr=0x{args.software_address:08X}, "
          f"port={args.port}, data={args.data.hex()}")

mem_obs = core.create_end_sw_memory_access_observer(
    cb=on_sw_mem_access,
    address=0x40000000,        # Start address to watch
    access_kind='any',
    region_size=0x1000         # Watch a 4KB region
)
```

---

### 4.7 Register Probing

The utility classes `RegisterFieldProbe` and `RegisterSetProbe` provide high-level access to SCML2 register fields:

```python
from sim_utils import RegisterFieldProbe, RegisterSetProbe

# Probe an entire register set
reg_set = RegisterSetProbe('top.peripheral.reg_set')

# Probe a specific register field
field = RegisterFieldProbe('top.peripheral.reg_set.CTRL.ENABLE')
val = field.get_value()
field.set_value(1)
```

---

### 4.8 GDA Trace Creation (Data Analysis)

SimProbe can create custom GDA (Generic Data Analysis) traces for visualization in Synopsys analysis tools:

```python
import sim

# Create an integer trace
trace = sim.IntegerTrace(
    owner='top.my_module',
    name='custom_counter',
    desc='Tracks a custom counter value'
)
el_id = trace.add_element('counter')

# Update trace values during simulation
trace.set_value(42, element_id=el_id)
sim.wait_for((1, 'us'))
trace.set_value(100, element_id=el_id)

# Enum traces for state machines
state_trace = sim.EnumTrace('top.fsm', 'state', 'FSM State')
state_trace.register_value(0, 'IDLE')
state_trace.register_value(1, 'ACTIVE')
state_trace.register_value(2, 'ERROR')
state_trace.set_value(0)  # Set initial state to IDLE
```

---

### 4.9 Profiling

```python
import sim

profiler = sim.Profiler()

# Start Top-Down Profiler collection
profiler.start_top_down_profile({
    'output_file': 'profile_results.db'
})

# Add segment annotations
seg_id = profiler.begin_segment_annotation('boot_phase')
# ... simulation runs ...
profiler.end_segment_annotation(seg_id)

# Stop profiling
profiler.stop_top_down_profile()
```

---

### 4.10 Simulation Control

```python
import sim

# Time management
current_time = sim.get_time()           # Formatted string
time_ps = sim.get_time_in_ps()          # Picoseconds as float
time_us = sim.get_time_in_unit('us')    # In microseconds

# Wait for simulation time
sim.wait_for((10, 'us'))                # Wait for 10 microseconds
sim.wait_until((1, 'ms'))              # Wait until simulation reaches 1ms

# Suspend/stop simulation
sim.suspend_simulation()                # Pause (resume from outside)
sim.stop_simulation()                   # End simulation

# Event-based synchronization
evt = sim.EventProbe('top.my_event', create=True)
evt.notify()                            # Notify event
evt.notify((5, 'ns'))                   # Timed notification
evt.wait()                              # Wait for notification (blocking)

# Inter-thread communication
sim.post_message("Hello from SimProbe")  # Send to VDK Debug console
sim.print_message("Thread-safe output")  # Thread-safe print

# Query simulation objects
top = sim.get_top_instance_name()
cores = sim.get_cores()
connected = sim.are_connected('top.a.socket', 'top.b.socket')
```

---

## 5. Complete Backdoor Memory Loading Examples

### Example 1: Full Firmware Load at Simulation Start

```python
#!/usr/bin/env python3
"""
SimProbe script: Load firmware into core memory at simulation begin.
Run via: sim_setup.create_standalone_sc_thread('load_firmware.py')
"""
import sim
from sim_utils import INFO_MSG, FATAL_MSG

def main():
    core_name = 'top.cpu0'
    firmware_path = 'firmware/app.elf'

    INFO_MSG(f"Loading firmware from {firmware_path}")

    try:
        core = sim.CoreProbe(core_name)

        # Method 1: Load ELF image (includes symbols)
        success = core.load_image(firmware_path, offset=0, debug_info=True)

        if success:
            INFO_MSG("Firmware loaded successfully via load_image()")

            # Verify: read the reset vector
            reset_vec = core.get_memory_value(0x00000000, 4)
            INFO_MSG(f"Reset vector: {reset_vec.hex()}")
        else:
            FATAL_MSG("Failed to load firmware")

    except RuntimeError as e:
        FATAL_MSG(f"Error loading firmware: {e}")

main()
```

### Example 2: Load Raw Binary Data via Backdoor

```python
#!/usr/bin/env python3
"""
SimProbe script: Load raw binary data into memory via CoreProbe backdoor.
"""
import sim
from sim_utils import INFO_MSG

def load_binary(core_name, file_path, base_addr):
    core = sim.CoreProbe(core_name)

    with open(file_path, 'rb') as f:
        data = bytearray(f.read())

    total = len(data)
    chunk_sz = 1024
    written = 0

    for off in range(0, total, chunk_sz):
        chunk = data[off:off + chunk_sz]
        core.set_memory_value(base_addr + off, chunk, len(chunk))
        written += len(chunk)

    INFO_MSG(f"Loaded {written} bytes to 0x{base_addr:08X}")

    # Verify first 16 bytes
    verify = core.get_memory_value(base_addr, 16)
    INFO_MSG(f"Verify [0x{base_addr:08X}]: {verify.hex()}")

    return written

# Load test data
load_binary('top.cpu0', 'test_vectors.bin', 0x20000000)
```

### Example 3: Load Data via Bus Debug Transactions

```python
#!/usr/bin/env python3
"""
SimProbe script: Load data into peripheral memory via bus debug transactions.
Useful when memory is behind an address decoder/interconnect.
"""
import sim
from sim_utils import BasicInitiatorSocketProbe, INFO_MSG

def load_via_bus(socket_name, base_addr, data_dict):
    """
    Load a dictionary of {offset: value} pairs via debug writes.
    """
    probe = BasicInitiatorSocketProbe(socket_name)

    for offset, value in data_dict.items():
        addr = base_addr + offset
        probe.debug_write4(addr, value)
        # Verify
        readback = probe.debug_read4(addr)
        assert readback == value, f"Mismatch at 0x{addr:08X}: wrote 0x{value:08X}, read 0x{readback:08X}"

    INFO_MSG(f"Loaded {len(data_dict)} words via {socket_name}")

# Example: Initialize PCIe configuration space registers
pcie_config = {
    0x00: 0x16C311AB,  # Vendor/Device ID
    0x04: 0x00100006,  # Command/Status
    0x08: 0x02800001,  # Class Code / Revision
    0x0C: 0x00000010,  # Header Type
    0x10: 0xF0000000,  # BAR0
    0x14: 0x00000000,  # BAR1
}

load_via_bus('top.bus.init_socket', 0xFE000000, pcie_config)
```

### Example 4: Memory Initialization with Observation

```python
#!/usr/bin/env python3
"""
SimProbe script: Initialize memory and set up observers to watch for accesses.
"""
import sim
from sim_utils import INFO_MSG

# Step 1: Initialize memory region via backdoor
core = sim.CoreProbe('top.cpu0')

# Write test pattern
for i in range(256):
    data = bytearray([(i >> 0) & 0xFF, (i >> 8) & 0xFF, 0x00, 0x00])
    core.set_memory_value(0x20000000 + i * 4, data, 4)

INFO_MSG("Memory initialized with test pattern")

# Step 2: Set up memory content observer to watch for accesses
def on_access(observer, args):
    val = args.get_value()
    INFO_MSG(f"Memory {args.access_kind} at index {args.index}: 0x{val:08X}")

obs = sim.MemoryContentObserver(
    'top.sram.memory',
    start=0, end=255,
    cb=on_access,
    access_kind='any'
)

INFO_MSG("Memory observer armed -- waiting for accesses")
```

---

## 6. Summary of Key APIs for Backdoor Access

| API Class / Method | Access Type | Use Case |
|---|---|---|
| **`CoreProbe.load_image()`** | Backdoor | Load complete ELF/HEX/SREC images with symbols |
| **`CoreProbe.set_memory_value()`** | Backdoor | Write raw data to core memory (debug write) |
| **`CoreProbe.get_memory_value()`** | Backdoor | Read raw data from core memory (debug read) |
| **`CoreProbe.set_software_data_view_memory_value()`** | Backdoor | Write via virtual address (MMU-aware) |
| **`CoreProbe.get_software_data_view_memory_value()`** | Backdoor | Read via virtual address (MMU-aware) |
| **`MemoryProbe.set_value()`** | Backdoor | Write to SCML2 memory by SystemC name (debug write) |
| **`MemoryProbe.get_value()`** | Backdoor | Read from SCML2 memory by SystemC name (debug read) |
| **`MemoryProbe.set_clamp_with_value()`** | Backdoor + Clamp | Force memory to return a specific value |
| **`InitiatorSocketProbe.debug_write()`** | Backdoor via Bus | Debug write transaction through TLM2 socket |
| **`InitiatorSocketProbe.debug_read()`** | Backdoor via Bus | Debug read transaction through TLM2 socket |
| **`BasicInitiatorSocketProbe.debug_write4()`** | Backdoor via Bus | Convenience 4-byte debug write |
| **`BasicInitiatorSocketProbe.mem_fill()`** | Backdoor via Bus | Fill memory region with pattern |
| **`BasicInitiatorSocketProbe.mem_copy()`** | Backdoor via Bus | Copy between memory regions |

### Key Distinctions

| Approach | When to Use |
|----------|-------------|
| **CoreProbe** | When loading firmware/data into a processor core's memory space. Supports ELF loading with symbols. |
| **MemoryProbe** | When accessing a specific SCML2 memory object by its SystemC hierarchical name. Supports clamping. |
| **InitiatorSocketProbe** | When accessing memory through the bus interconnect (address-decoded). Useful for peripherals. |

### Backdoor vs. Normal Access

| Property | Backdoor (Debug) | Normal (Timed) |
|----------|-------------------|-----------------|
| Simulation time | Zero-delay | Consumes time |
| Bus arbitration | Bypassed | Normal arbitration |
| Side effects | None | Triggers callbacks, interrupts |
| Cache coherency | Not affected | Normal behavior |
| Use case | Initialization, inspection | Functional testing |

---

*Document generated from: VP Simulation Probes Python Interface Reference Manual, V-2024.03, April 2024, Synopsys, Inc.*
