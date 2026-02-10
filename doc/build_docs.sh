#!/bin/bash
# Automated Sphinx Documentation Build Script
# Builds beautiful HTML documentation with Mermaid diagrams

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Keraunos PCIe Tile Documentation Builder${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: Python 3 is not installed${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} Python 3 found: $(python3 --version)"

# Check if pip is available
if ! command -v pip3 &> /dev/null; then
    echo -e "${RED}Error: pip3 is not installed${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} pip3 found"

# Install dependencies if needed
echo ""
echo -e "${YELLOW}Checking dependencies...${NC}"

if python3 -c "import sphinx" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Sphinx already installed"
else
    echo -e "${YELLOW}Installing Sphinx and extensions...${NC}"
    
    # Try standard requirements first
    if pip3 install -r requirements.txt 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Dependencies installed"
    else
        echo -e "${YELLOW}Standard installation failed, trying minimal requirements...${NC}"
        if pip3 install -r requirements-minimal.txt; then
            echo -e "${GREEN}✓${NC} Minimal dependencies installed"
        else
            echo -e "${RED}✗${NC} Failed to install dependencies"
            echo -e "${YELLOW}Please install manually:${NC}"
            echo -e "  pip3 install sphinx sphinx_rtd_theme myst-parser sphinxcontrib-mermaid"
            exit 1
        fi
    fi
fi

# Check for required extensions
echo ""
echo -e "${YELLOW}Verifying extensions...${NC}"

check_module() {
    if python3 -c "import $1" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} $2"
        return 0
    else
        echo -e "${RED}✗${NC} $2 not found"
        return 1
    fi
}

all_ok=true
check_module "sphinx" "Sphinx" || all_ok=false
check_module "sphinx_rtd_theme" "Read the Docs Theme" || all_ok=false
check_module "myst_parser" "MyST Parser" || all_ok=false
check_module "sphinxcontrib.mermaid" "Mermaid Extension" || all_ok=false

if [ "$all_ok" = false ]; then
    echo ""
    echo -e "${YELLOW}Installing missing dependencies...${NC}"
    
    # Try standard requirements first
    if pip3 install -r requirements.txt 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Dependencies installed successfully"
    else
        echo -e "${YELLOW}Standard installation failed, trying minimal requirements...${NC}"
        if pip3 install -r requirements-minimal.txt; then
            echo -e "${GREEN}✓${NC} Minimal dependencies installed"
        else
            echo -e "${RED}✗${NC} Installation failed. Please install manually:"
            echo -e "  ${YELLOW}pip3 install sphinx sphinx_rtd_theme myst-parser sphinxcontrib-mermaid${NC}"
            exit 1
        fi
    fi
fi

# Clean previous build
echo ""
echo -e "${YELLOW}Cleaning previous build...${NC}"
if [ -d "_build" ]; then
    rm -rf _build
    echo -e "${GREEN}✓${NC} Cleaned _build directory"
else
    echo -e "${BLUE}ℹ${NC} No previous build to clean"
fi

# Create _static directory if it doesn't exist
if [ ! -d "_static" ]; then
    mkdir -p _static
    echo -e "${GREEN}✓${NC} Created _static directory"
fi

# Build HTML documentation
echo ""
echo -e "${YELLOW}Building HTML documentation...${NC}"
echo -e "${BLUE}----------------------------------------${NC}"

if sphinx-build -b html -W --keep-going . _build/html; then
    echo -e "${BLUE}----------------------------------------${NC}"
    echo -e "${GREEN}✓${NC} Documentation built successfully!"
else
    echo -e "${BLUE}----------------------------------------${NC}"
    echo -e "${RED}✗${NC} Build failed with errors"
    exit 1
fi

# Check output size
BUILD_SIZE=$(du -sh _build/html | cut -f1)
NUM_FILES=$(find _build/html -type f | wc -l)
NUM_DIAGRAMS=$(grep -r "mermaid" Keraunos_PCIE_Tile_SystemC_Design_Document.md | wc -l)

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Build Statistics:${NC}"
echo -e "  Output size:      ${BUILD_SIZE}"
echo -e "  Files generated:  ${NUM_FILES}"
echo -e "  Mermaid diagrams: ${NUM_DIAGRAMS}"
echo -e "${BLUE}========================================${NC}"

# Show output location
echo ""
echo -e "${GREEN}Documentation available at:${NC}"
echo -e "  ${BLUE}file://$(pwd)/_build/html/index.html${NC}"
echo ""

# Offer to open in browser
read -p "Open documentation in browser? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if command -v xdg-open &> /dev/null; then
        xdg-open _build/html/index.html
    elif command -v open &> /dev/null; then
        open _build/html/index.html
    else
        echo -e "${YELLOW}Could not detect browser command. Please open manually:${NC}"
        echo -e "  ${BLUE}_build/html/index.html${NC}"
    fi
fi

echo ""
echo -e "${GREEN}✓${NC} Done! Enjoy your beautiful documentation with Mermaid diagrams! 🎉"
