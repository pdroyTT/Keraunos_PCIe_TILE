#!/bin/bash
#
# Simple PDF to Markdown converter using system tools
# Extracts text and images from PDF and creates Markdown with image references
#
# Usage: ./convert_pdf_simple.sh input.pdf [output.md]

set -e

INPUT_PDF="$1"
OUTPUT_MD="${2:-${INPUT_PDF%.pdf}.md}"

if [ -z "$INPUT_PDF" ]; then
    echo "Usage: $0 input.pdf [output.md]"
    exit 1
fi

if [ ! -f "$INPUT_PDF" ]; then
    echo "Error: PDF file not found: $INPUT_PDF"
    exit 1
fi

# Get base name and directory
BASENAME=$(basename "$INPUT_PDF" .pdf)
DIRNAME=$(dirname "$INPUT_PDF")
OUTPUT_DIR=$(dirname "$OUTPUT_MD")
IMAGES_DIR="${OUTPUT_DIR}/images"

# Create output directory and images directory
mkdir -p "$OUTPUT_DIR"
mkdir -p "$IMAGES_DIR"

echo "Converting PDF to Markdown..."
echo "Input:  $INPUT_PDF"
echo "Output: $OUTPUT_MD"
echo "Images: $IMAGES_DIR"
echo ""

# Extract text using pdftotext
echo "Extracting text..."
pdftotext -layout "$INPUT_PDF" - | sed 's/\x0c//' > "${OUTPUT_MD}.tmp"

# Extract images using pdfimages
echo "Extracting images..."
cd "$IMAGES_DIR"
pdfimages -all "$(realpath "$INPUT_PDF")" "${BASENAME}_img" 2>/dev/null || {
    echo "Warning: pdfimages failed, trying pdftoppm..."
    # Alternative: use pdftoppm to convert pages to images
    if command -v pdftoppm &> /dev/null; then
        pdftoppm -png -r 150 "$(realpath "$INPUT_PDF")" "${BASENAME}_page" 2>/dev/null || true
    fi
}
cd - > /dev/null

# Count extracted images
IMAGE_COUNT=$(find "$IMAGES_DIR" -name "${BASENAME}_img*" -o -name "${BASENAME}_page*" 2>/dev/null | wc -l)
echo "Extracted $IMAGE_COUNT images"

# Create Markdown header
cat > "$OUTPUT_MD" << EOF
# ${BASENAME//_/ }

*Converted from PDF: $(basename "$INPUT_PDF")*

---

EOF

# Add text content
cat "${OUTPUT_MD}.tmp" >> "$OUTPUT_MD"

# Add image references at the end
if [ "$IMAGE_COUNT" -gt 0 ]; then
    echo "" >> "$OUTPUT_MD"
    echo "---" >> "$OUTPUT_MD"
    echo "" >> "$OUTPUT_MD"
    echo "## Diagrams and Images" >> "$OUTPUT_MD"
    echo "" >> "$OUTPUT_MD"
    
    # Add image references
    find "$IMAGES_DIR" -name "${BASENAME}_img*" -o -name "${BASENAME}_page*" 2>/dev/null | sort | while read img; do
        IMG_NAME=$(basename "$img")
        REL_PATH="images/$IMG_NAME"
        echo "![$IMG_NAME]($REL_PATH)" >> "$OUTPUT_MD"
        echo "" >> "$OUTPUT_MD"
    done
fi

# Cleanup temp file
rm -f "${OUTPUT_MD}.tmp"

echo ""
echo "✅ Conversion complete!"
echo "📄 Output: $OUTPUT_MD"
if [ "$IMAGE_COUNT" -gt 0 ]; then
    echo "🖼️  Images: $IMAGES_DIR ($IMAGE_COUNT images)"
fi
