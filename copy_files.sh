#!/bin/bash

# Script to copy header and source files
# Header files: include/ -> Keraunos_PCIe_tile/SystemC/include/
# Source files: src/ -> Keraunos_PCIe_tile/SystemC/src/

set -e  # Exit on error

# Get the script directory (workspace root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Source and destination directories
INCLUDE_SRC="include"
INCLUDE_DST="Keraunos_PCIe_tile/SystemC/include"

SRC_SRC="src"
SRC_DST="Keraunos_PCIe_tile/SystemC/src"

# Function to copy files
copy_files() {
    local source_dir="$1"
    local dest_dir="$2"
    local file_pattern="$3"
    local file_type="$4"
    
    echo "Processing $file_type files..."
    echo "  Source: $source_dir/"
    echo "  Destination: $dest_dir/"
    
    # Check if source directory exists
    if [ ! -d "$source_dir" ]; then
        echo "  ERROR: Source directory '$source_dir' does not exist!"
        return 1
    fi
    
    # Create destination directory if it doesn't exist
    mkdir -p "$dest_dir"
    
    # Count files processed
    local count=0
    local skipped=0
    local overwritten=0
    
    # Process each file
    for file in "$source_dir"/$file_pattern; do
        # Check if file exists (handles case where no files match)
        [ -e "$file" ] || continue
        
        filename=$(basename "$file")
        dest_file="$dest_dir/$filename"
        
        # Check if destination already exists
        if [ -e "$dest_file" ] || [ -L "$dest_file" ]; then
            # Remove existing file/symlink before copying
            rm -f "$dest_file"
            overwritten=$((overwritten + 1))
        fi
        
        # Copy file
        cp "$file" "$dest_file"
        echo "  COPY: $filename"
        count=$((count + 1))
    done
    
    echo "  Summary: Copied $count files"
    if [ $overwritten -gt 0 ]; then
        echo "           Overwritten $overwritten existing files"
    fi
    echo ""
}

# Main execution
echo "=========================================="
echo "Copying Files for Keraunos PCIe Tile"
echo "=========================================="
echo ""

# Copy header files
copy_files "$INCLUDE_SRC" "$INCLUDE_DST" "*.h" "Header"

# Copy source files
copy_files "$SRC_SRC" "$SRC_DST" "*.cpp" "Source"

echo "=========================================="
echo "File copy complete!"
echo "=========================================="
