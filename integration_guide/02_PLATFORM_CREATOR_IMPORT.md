# Step 2: Import SystemC IP into Platform Creator

## Overview

This guide shows how to import your prepared SystemC IP into Synopsys Platform Creator (PCT) and create a reusable component.

**Time Required:** 1-4 hours

**Tool Used:** Platform Creator (`pct.exe`)

---

## Prerequisites

✅ **Before You Start:**
- [ ] Completed Step 1 (IP Preparation)
- [ ] SystemC IP compiles without errors
- [ ] TLM 2.0 sockets are implemented
- [ ] Virtualizer environment is set up

---

## Step 2.1: Launch Platform Creator

### Setup Environment

```bash
# Navigate to workspace
cd /localdev/pdroy/keraunos_pcie_workspace

# Setup Virtualizer (fixes Qt issues)
source ./setup_virtualizer.sh

# Launch Platform Creator
pct.exe &
```

**Alternative: Direct Launch**
```bash
export LD_LIBRARY_PATH=/tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux/tools/libso:$LD_LIBRARY_PATH
source /tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03/SLS/linux/setup.sh -vze
pct.exe &
```

### Wait for GUI

Platform Creator GUI should appear. If you see Qt errors, refer to `VIRTUALIZER_QT_FIX.md`.

---

## Step 2.2: Create New Component Library

### Option A: Create New Library

1. **File → New → Component Library**
2. **Library Settings:**
   - **Name:** `my_systemc_ips`
   - **Location:** `/localdev/pdroy/keraunos_pcie_workspace/pct_library`
   - **Description:** "Custom SystemC IP Components"
3. Click **OK**

### Option B: Use Existing Library

1. **File → Open Library**
2. Navigate to existing `.pctlib` file
3. Click **Open**

---

## Step 3: Import SystemC Component

### Method 1: Import SystemC Source (Recommended)

**Steps:**

1. **Right-click library → New → Component from SystemC**

2. **Component Creation Wizard appears:**

   **Page 1: Component Information**
   - **Component Name:** `inbound_tlb`
   - **Module Name:** `inbound_tlb` (must match C++ class name)
   - **Description:** "Inbound TLB for Device IOVA translation"
   - Click **Next**

   **Page 2: SystemC Source Files**
   - Click **Add Files**
   - Select:
     - `inbound_tlb.h`
     - `inbound_tlb.cpp`
   - **Include Directories:** Add any additional include paths
   - Click **Next**

   **Page 3: Compilation Settings**
   - **Compiler:** GCC
   - **C++ Standard:** C++17
   - **Additional Flags:** `-Wall -Wextra`
   - **Defines:** (if any) e.g., `DEBUG_MODE`
   - Click **Next**

   **Page 4: Port Detection**
   - Platform Creator will scan for TLM sockets
   - Verify detected ports:
     ```
     [✓] target_socket    (TLM Target)
     [✓] initiator_socket (TLM Initiator)
     ```
   - Click **Next**

   **Page 5: Parameters**
   - Add configurable parameters:
     ```
     num_entries  (int, default: 64)
     page_size    (int, default: 4096)
     debug_mode   (bool, default: false)
     ```
   - Click **Finish**

3. **Component Created!**
   - Component appears in library tree
   - Icon shows TLM sockets

---

## Step 2.4: Configure Component Properties

### Open Component Editor

Double-click component in library to open editor.

### General Tab

**Component Information:**
- **Display Name:** "Inbound TLB"
- **Version:** "1.0"
- **Vendor:** "Your Company"
- **Description:** Detailed description

### Ports Tab

**Verify/Edit TLM Sockets:**

**Target Socket:**
- **Name:** `target_socket`
- **Type:** TLM 2.0 Target
- **Protocol:** Generic Payload
- **Address Width:** 64 bits
- **Data Width:** 32 bits (or your data width)

**Initiator Socket:**
- **Name:** `initiator_socket`
- **Type:** TLM 2.0 Initiator
- **Protocol:** Generic Payload
- **Address Width:** 64 bits
- **Data Width:** 32 bits

**Add Clock/Reset (Optional):**
If your IP needs clock/reset:
- Right-click → Add Port
- Type: `sc_in<bool>` for reset
- Type: `sc_in<sc_clock>` for clock

### Parameters Tab

**Edit Parameters:**

For each parameter:
```
Parameter: num_entries
  Type: Integer
  Default: 64
  Min: 1
  Max: 1024
  Description: "Number of TLB entries"
```

### Address Map Tab

**Define Register Map:**

If your IP is memory-mapped:

1. **Add Address Space**
   - Name: `registers`
   - Size: `0x1000` (4KB)
   - Base Address: `0x0` (relative)

2. **Add Registers**
   ```
   Register: CTRL (0x00, 4 bytes, RW)
   Register: STATUS (0x04, 4 bytes, RO)
   Register: ADDR_IN (0x08, 8 bytes, RW)
   Register: ADDR_OUT (0x10, 8 bytes, RO)
   ```

### Documentation Tab

**Add Documentation:**
- **Overview:** What the component does
- **Usage:** How to connect it
- **Registers:** Register descriptions
- **Examples:** Code snippets

---

## Step 2.5: Validate Component

### Run Built-in Validation

1. **Tools → Validate Component**
2. Check for:
   - ✓ Port consistency
   - ✓ Parameter types
   - ✓ Address map conflicts
   - ✓ Source file accessibility

### Fix Common Validation Errors

**Error: "Source file not found"**
```
Solution: Add correct include directories in Compilation Settings
```

**Error: "Port type mismatch"**
```
Solution: Ensure TLM socket types match in code and PCT
```

**Error: "Multiple definitions"**
```
Solution: Check for duplicate .cpp files or missing include guards
```

---

## Step 2.6: Test Compile Component

### Manual Test Build

Before proceeding, test if component compiles:

1. **Right-click component → Build**
2. **Or:** Tools → Build Component

### Check Build Output

**Success:**
```
Building component 'inbound_tlb'...
Compiling inbound_tlb.cpp...
Component built successfully
```

**Failure:**
```
Error: undefined reference to 'tlm::...'
```

**Common Solutions:**
- Add SystemC/TLM include paths
- Link with `-lsystemc`
- Check C++ standard (must be C++17)

---

## Step 2.7: Add Component Icon (Optional)

### Custom Component Icon

1. **Right-click component → Properties**
2. **Icon Tab**
3. **Browse** for icon file (PNG, 32x32 or 64x64)
4. Or use default PCT icon

**Icon File Location:**
```
/localdev/pdroy/keraunos_pcie_workspace/icons/tlb_icon.png
```

---

## Step 2.8: Save and Export Library

### Save Library

**File → Save Library**

Library saved as:
```
/localdev/pdroy/keraunos_pcie_workspace/pct_library/my_systemc_ips.pctlib
```

### Export for Sharing (Optional)

**File → Export Library**

Creates distributable package:
```
my_systemc_ips_v1.0.zip
```

---

## Step 2.9: Create Multiple Component Variants

If you have multiple TLB types:

### Variant 1: Inbound TLB
```
Component: inbound_tlb
Purpose: Device → Memory translation
Sockets: 1 target (from devices), 1 initiator (to memory)
```

### Variant 2: Outbound TLB
```
Component: outbound_tlb
Purpose: CPU → Memory translation
Sockets: 1 target (from CPU), 1 initiator (to memory)
```

### Create Second Component

Repeat Steps 2.3-2.8 for each variant.

---

## Troubleshooting Platform Creator

### Issue: PCT Won't Launch

**Symptom:** Qt library errors

**Solution:**
```bash
# Use the wrapper script
source /localdev/pdroy/keraunos_pcie_workspace/setup_virtualizer.sh
pct.exe
```

### Issue: Component Won't Build

**Check Compilation Settings:**
1. Open component properties
2. Go to **Build → Compilation**
3. Verify:
   - SystemC include path: `/home/pdroy/systemc-local/include`
   - TLM include path: (included in SystemC)
   - C++ Standard: C++17 or later

### Issue: Sockets Not Detected

**Manual Socket Addition:**
1. Open component editor
2. **Ports Tab**
3. **Add Port** button
4. Configure manually:
   - Type: TLM 2.0 Target/Initiator
   - Protocol: tlm_generic_payload
   - Name: Must match code

### Issue: Cannot Find Header Files

**Add Include Directories:**
1. Component Properties
2. **Build → Includes**
3. Add paths:
   ```
   /home/pdroy/systemc-local/include
   /localdev/pdroy/keraunos_pcie_workspace/common_headers
   ```

---

## Platform Creator Quick Reference

### Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Save | `Ctrl+S` |
| Build Component | `Ctrl+B` |
| Validate | `Ctrl+V` |
| Find | `Ctrl+F` |

### File Structure Created

```
pct_library/
├── my_systemc_ips.pctlib           (Library metadata)
├── inbound_tlb/
│   ├── component.xml               (Component config)
│   ├── inbound_tlb.h              (Source)
│   ├── inbound_tlb.cpp            (Source)
│   └── docs/                      (Documentation)
└── outbound_tlb/
    └── ... (similar structure)
```

---

## Next Steps

✅ **Component successfully imported into Platform Creator!**

→ **Proceed to:** `03_TLM_INTERFACE_SETUP.md`

This will guide you through configuring TLM socket connections and memory mapping.

---

## Checklist

Before moving to next step, verify:

- [ ] Component imports without errors
- [ ] All TLM sockets are detected/configured
- [ ] Component builds successfully in PCT
- [ ] Parameters are properly defined
- [ ] Address map is configured (if memory-mapped)
- [ ] Component is saved in library

---

**Platform Creator Tips:**

💡 Save frequently - PCT can crash on complex imports  
💡 Keep component names simple (no spaces/special chars)  
💡 Document everything in the Documentation tab  
💡 Test build before adding to VDK

