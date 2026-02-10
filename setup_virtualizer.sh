#!/bin/bash
# Synopsys Virtualizer Setup Script with Qt Library Fix

echo "Setting up Synopsys Virtualizer V-2024.03..."

# Tool installation path
VIRTUALIZER_HOME=/tools_vendor/synopsys/virtualizer-tool-elite/V-2024.03

# Priority 1: Tool's bundled Qt libraries (must come FIRST)
export LD_LIBRARY_PATH=${VIRTUALIZER_HOME}/SLS/linux/tools/libso:${LD_LIBRARY_PATH}

# Priority 2: Tool's other libraries
export LD_LIBRARY_PATH=${VIRTUALIZER_HOME}/SLS/linux/pc/lib:${LD_LIBRARY_PATH}

# Source the tool's setup script
source ${VIRTUALIZER_HOME}/SLS/linux/setup.sh -vze

echo "✓ Virtualizer setup complete"
echo ""
echo "Library path configured to use tool's bundled Qt libraries"
echo ""
echo "You can now run:"
echo "  pct.exe"
echo "  vpc"
echo "  etc."

