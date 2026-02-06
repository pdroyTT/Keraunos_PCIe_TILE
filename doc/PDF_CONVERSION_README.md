# PDF to Markdown Conversion Scripts

## Overview

This directory contains scripts to convert PDF files to Markdown format while preserving diagrams and images.

## Available Scripts

### 1. `pdf_to_markdown_with_diagrams.py` (Recommended)

**Python script** that converts PDF to Markdown with full diagram preservation.

**Features:**
- ✅ Extracts text from PDF pages
- ✅ Extracts all images/diagrams from PDF
- ✅ Preserves document structure (headings, lists, tables)
- ✅ Embeds images in Markdown format
- ✅ Maintains page layout information
- ✅ Works with system tools (pdftotext, pdfimages) - no Python dependencies required

**Usage:**
```bash
# Convert PDF to Markdown
python3 pdf_to_markdown_with_diagrams.py doc/keraunos_pcie_tile.pdf

# Specify output file
python3 pdf_to_markdown_with_diagrams.py input.pdf -o output.md

# Check dependencies
python3 pdf_to_markdown_with_diagrams.py --check-deps
```

**Output:**
- Creates `keraunos_pcie_tile.md` (Markdown file)
- Creates `images/` directory with all extracted diagrams
- Images are referenced in Markdown as `![alt text](images/filename.png)`

**Requirements:**
- Python 3.6+
- System tools: `pdftotext`, `pdfimages` (usually installed with poppler-utils)
- Optional: PyMuPDF (for better results) - `pip install PyMuPDF`

---

### 2. `convert_pdf_simple.sh` (Alternative)

**Bash script** that uses system tools directly.

**Usage:**
```bash
./convert_pdf_simple.sh doc/keraunos_pcie_tile.pdf
./convert_pdf_simple.sh input.pdf output.md
```

**Requirements:**
- `pdftotext` (from poppler-utils)
- `pdfimages` (from poppler-utils)

---

## Example: Converting keraunos_pcie_tile.pdf

### Using Python Script:

```bash
cd /localdev/pdroy/keraunos_pcie_workspace/doc
python3 pdf_to_markdown_with_diagrams.py keraunos_pcie_tile.pdf
```

**Output:**
```
📄 Input PDF:  /localdev/pdroy/keraunos_pcie_workspace/doc/keraunos_pcie_tile.pdf
📝 Output MD:  /localdev/pdroy/keraunos_pcie_workspace/doc/keraunos_pcie_tile.md
📁 Output dir: /localdev/pdroy/keraunos_pcie_workspace/doc

🔄 Processing PDF...
Using system tools (pdftotext/pdfimages) for PDF processing...
✅ Processed 28 pages
🖼️  Extracted 38 images to /localdev/pdroy/keraunos_pcie_workspace/doc/images
📝 Generating Markdown...
✅ Markdown saved to: /localdev/pdroy/keraunos_pcie_workspace/doc/keraunos_pcie_tile.md
📊 Output size: 82,712 bytes

✅ Conversion complete!
```

### Result:

- **Markdown file:** `keraunos_pcie_tile.md` (1,972 lines, 82KB)
- **Images directory:** `images/` (38 images, 4.8MB)
- **Images referenced:** All diagrams are embedded in Markdown with proper references

---

## How It Works

### Text Extraction:
1. Uses `pdftotext` to extract text content from PDF
2. Preserves layout and formatting
3. Detects headings and converts to Markdown headers
4. Splits content by pages

### Image Extraction:
1. Uses `pdfimages` to extract all images from PDF
2. Saves images as PNG files in `images/` directory
3. Images are named: `{pdf_name}_img-{number}.png`

### Markdown Generation:
1. Creates Markdown header with PDF name
2. Processes text line by line:
   - Detects headings (all caps, numbered sections)
   - Converts to Markdown headers (#, ##, ###)
   - Preserves regular text
3. Adds image references at appropriate locations
4. Maintains page separators

---

## Image References in Markdown

Images are referenced in the Markdown file like this:

```markdown
![Page 1, Image 1](images/keraunos_pcie_tile_img-000.png)

*Image dimensions: 800x600 pixels*
```

---

## Troubleshooting

### Issue: "pdftotext not found"
**Solution:** Install poppler-utils:
```bash
# Red Hat/CentOS
sudo yum install poppler-utils

# Ubuntu/Debian
sudo apt-get install poppler-utils
```

### Issue: "pdfimages not found"
**Solution:** Same as above - install poppler-utils

### Issue: Images not extracted
**Solution:** 
- Check if PDF contains embedded images (some PDFs have vector graphics)
- Try using PyMuPDF for better image extraction:
  ```bash
  pip install PyMuPDF
  ```

### Issue: Text formatting is poor
**Solution:**
- The script preserves layout as much as possible
- For better results, use PyMuPDF which has better text extraction
- Manual editing may be needed for complex layouts

---

## File Structure After Conversion

```
doc/
├── keraunos_pcie_tile.pdf          # Original PDF
├── keraunos_pcie_tile.md           # Generated Markdown
├── images/                          # Extracted images
│   ├── keraunos_pcie_tile_img-000.png
│   ├── keraunos_pcie_tile_img-001.png
│   └── ...
└── pdf_to_markdown_with_diagrams.py # Conversion script
```

---

## Notes

- **Page count:** The script automatically detects the number of pages
- **Image format:** Images are extracted as PNG by default
- **Text encoding:** UTF-8 encoding is used for text extraction
- **Large PDFs:** The script handles large PDFs (tested with 5.4MB, 28 pages)
- **Diagram preservation:** All diagrams are faithfully extracted and referenced

---

## Successfully Converted

✅ **keraunos_pcie_tile.pdf** → **keraunos_pcie_tile.md**
- 28 pages converted
- 38 images/diagrams extracted
- All diagrams preserved and referenced in Markdown

---

**Created:** January 29, 2026  
**Last Updated:** January 29, 2026
