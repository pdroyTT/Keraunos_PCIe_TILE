#!/bin/bash
#
# Publish Documentation to GitHub Pages
# Run this after making changes to documentation
#

set -e

echo "=================================================="
echo "Publishing Keraunos PCIe Tile Documentation"
echo "=================================================="
echo ""

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "Step 1: Rebuild Sphinx Documentation"
echo "------------------------------------"

cd docs
echo "Cleaning previous build..."
rm -rf _build/html
echo "Building HTML documentation..."
sphinx-build -b html . _build/html
cd ..

echo -e "${GREEN}✅ HTML documentation rebuilt${NC}"
echo ""

echo "Step 2: Initialize/Update gh-pages Directory"
echo "--------------------------------------------"

# Create gh-pages directory if it doesn't exist
if [ ! -d "gh-pages" ]; then
    echo "Creating gh-pages directory..."
    mkdir -p gh-pages
    echo -e "${BLUE}Created gh-pages directory${NC}"
fi

# Remove old content (but keep .git if it exists)
if [ -d "gh-pages/.git" ]; then
    find gh-pages -mindepth 1 -maxdepth 1 ! -name '.git' -exec rm -rf {} +
else
    rm -rf gh-pages/*
fi

# Copy new HTML
echo "Copying HTML documentation..."
cp -r docs/_build/html/* gh-pages/

# Ensure .nojekyll exists (tells GitHub Pages not to use Jekyll)
touch gh-pages/.nojekyll

# Create CNAME file if needed (uncomment and set your custom domain)
# echo "your-domain.com" > gh-pages/CNAME

# Update README with comprehensive info
cat > gh-pages/README.md << 'EOF'
# Keraunos PCIe Tile Documentation

**Professional HTML Documentation with Interactive Diagrams**

This directory contains the complete Sphinx-generated HTML documentation for the Keraunos PCIe Tile SystemC/TLM2.0 implementation.

## 📚 View Online

**[View Documentation](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)**

## 📖 Documentation Contents

### Main Documents

1. **[High-Level Design (HLD)](hld.html)**
   - Architecture overview with 16 interactive Mermaid diagrams
   - Block diagrams and data flow sequences
   - Module descriptions and interface specifications
   - Complete address maps
   - Clock and reset strategy

2. **[SystemC Implementation Design](systemc_design.html)**
   - Detailed low-level SystemC/TLM2.0 design (~5100 lines)
   - Class implementations and API specifications
   - SCML2 memory models
   - Callback architectures

3. **[Test Plan & Verification](testplan.html)**
   - 76 test cases fully documented
   - Test traceability matrix
   - 251 assertions, 0 failures
   - 100% functional coverage

### Key Features

- ✅ **16 Interactive Mermaid Diagrams**
  - Sequence diagrams for data flows
  - State machines for control logic
  - Architecture and block diagrams
  - Test coverage visualizations

- ✅ **Enhanced Visualization**
  - Large diagrams (800-1200px)
  - 16-18px fonts for readability
  - Professional styling
  - Responsive design

- ✅ **Comprehensive Coverage**
  - 3 fabric switches (NOC-PCIE, NOC-IO, SMN-IO)
  - 8 TLBs (5 inbound + 3 outbound)
  - MSI-X relay with 16 vectors
  - System Information Interface (SII)
  - Complete verification results

## 🔄 Last Updated

**Date:** $(date '+%Y-%m-%d %H:%M:%S')  
**Build Tool:** Sphinx 5.3.0 with Mermaid 10.6.1  
**Status:** Production Ready ✅

## 📊 Statistics

- **Total Pages:** 4 main documents + index
- **Total Size:** $(du -sh . 2>/dev/null | cut -f1 || echo "N/A")
- **Diagrams:** 16 interactive Mermaid diagrams
- **Test Cases:** 76 (41 E2E + 35 directed)
- **Assertions:** 251 (0 failures)
- **Code Coverage:** 100% functional coverage

## 🎨 Features

### Mermaid Diagrams
- Top-level architecture
- Complete system architecture (Section 3.1 - EXTRA LARGE)
- 5 data flow sequence diagrams
- State machines (CII tracking)
- Clock and reset hierarchies
- Test distribution charts

### Professional Styling
- Responsive design
- Color-coded tables and diagrams
- Full-text search
- Clean navigation sidebar
- Print-friendly layouts

## 📝 Documentation Sources

Generated from markdown sources in:
- `/docs/hld.md` - High-Level Design
- `/docs/systemc_design.md` - SystemC Implementation
- `/docs/testplan.md` - Test Plan & Verification

## 🛠️ Build Information

- **Sphinx Version:** 5.3.0
- **Mermaid Version:** 10.6.1 (via sphinxcontrib-mermaid 0.7.1)
- **Theme:** Alabaster with custom CSS
- **Extensions:** myst_parser, sphinxcontrib.mermaid

## 📂 Repository

Source code and documentation: [Keraunos_PCIe_TILE](https://github.com/pdroyTT/Keraunos_PCIe_TILE)

---

*Auto-generated from markdown sources using Sphinx*
EOF

echo -e "${GREEN}✅ Updated gh-pages directory${NC}"
echo ""

echo "Step 3: Git Operations"
echo "---------------------"

# Initialize git if not already initialized
if [ ! -d ".git" ]; then
    echo -e "${YELLOW}Initializing git repository...${NC}"
    git init
    git add .
    git commit -m "Initial commit - Keraunos PCIe Tile Documentation"
    echo -e "${GREEN}✅ Git repository initialized${NC}"
else
    echo "Git repository already exists"
fi

# Add changes
git add gh-pages/
git add docs/hld.md docs/systemc_design.md docs/testplan.md docs/conf.py docs/_static/custom.css
git add doc/Keraunos_PCIe_Tile_HLD.md doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md doc/Keraunos_PCIE_Tile_Testplan.md

# Check if there are changes
if git diff --staged --quiet; then
    echo -e "${YELLOW}No changes to commit${NC}"
    echo ""
    echo -e "${BLUE}Documentation is up to date in gh-pages directory${NC}"
    echo "To view locally: firefox gh-pages/index.html"
    exit 0
fi

# Commit with detailed message
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
DOC_SIZE=$(du -sh gh-pages 2>/dev/null | cut -f1 || echo "N/A")

git commit -m "Update documentation - ${TIMESTAMP}

Comprehensive Keraunos PCIe Tile Documentation Update:

📚 Documentation Contents:
- High-Level Design with 16 Mermaid diagrams
- SystemC Implementation Design (~5100 lines)
- Test Plan with 76 test cases (251 assertions, 0 failures)

🎨 Enhancements:
- Interactive Mermaid diagrams (800-1200px, 16-18px fonts)
- Professional styling and responsive design
- Full-text search and navigation
- 100% functional test coverage

📊 Statistics:
- Total size: ${DOC_SIZE}
- 16 interactive diagrams
- 3 main documents + index
- Production ready with Sphinx 5.3.0

View at: https://pdroyTT.github.io/Keraunos_PCIe_TILE/"

echo -e "${GREEN}✅ Changes committed${NC}"
echo ""

# Check if remote exists
if git remote get-url origin &>/dev/null; then
    # Push to GitHub
    echo "Pushing to GitHub..."
    
    # Get current branch
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    
    git push -u origin ${BRANCH}
    
    echo ""
    echo -e "${GREEN}✅ Documentation published to GitHub!${NC}"
    echo ""
    echo "View at: https://pdroyTT.github.io/Keraunos_PCIe_TILE/"
    echo ""
    echo "Note: GitHub Pages may take 1-2 minutes to update"
else
    echo -e "${YELLOW}No git remote configured.${NC}"
    echo ""
    echo "To publish to GitHub Pages:"
    echo "1. Create a repository on GitHub: Keraunos_PCIe_TILE"
    echo "2. Add remote: git remote add origin https://github.com/pdroyTT/Keraunos_PCIe_TILE.git"
    echo "3. Push: git push -u origin main"
    echo "4. Enable GitHub Pages in repository settings (use 'gh-pages' branch or 'root' of main)"
    echo ""
    echo -e "${BLUE}Documentation is ready in gh-pages directory${NC}"
    echo "To view locally: firefox gh-pages/index.html"
fi

echo ""
echo "=================================================="
echo -e "${GREEN}Documentation Build Complete!${NC}"
echo "=================================================="
