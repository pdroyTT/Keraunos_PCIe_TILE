#!/bin/bash
#
# Setup GitHub Repository for Keraunos PCIe Tile Documentation
# This script creates the repository and sets up GitHub Pages
#

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo "=================================================="
echo "GitHub Repository Setup"
echo "Keraunos PCIe Tile Documentation"
echo "=================================================="
echo ""

# Configuration
GITHUB_USER="pdroyTT"
REPO_NAME="Keraunos_PCIe_TILE"
REPO_URL="https://github.com/${GITHUB_USER}/${REPO_NAME}.git"

echo -e "${BLUE}Repository: ${REPO_NAME}${NC}"
echo -e "${BLUE}GitHub User: ${GITHUB_USER}${NC}"
echo ""

# Check if git is configured
if ! git config user.name &>/dev/null; then
    echo "Configuring git user..."
    git config --global user.name "Prasanth Dhanasekar Roy"
    git config --global user.email "pdroy@tenstorrent.com"
    echo -e "${GREEN}✅ Git user configured${NC}"
fi

# Initialize git repository if needed
if [ ! -d ".git" ]; then
    echo "Initializing git repository..."
    git init
    git checkout -b main 2>/dev/null || git branch -M main
    echo -e "${GREEN}✅ Git repository initialized${NC}"
else
    echo "Git repository already exists"
fi

# Create .gitignore if it doesn't exist
if [ ! -f ".gitignore" ]; then
    cat > .gitignore << 'EOF'
# Build directories
docs/_build/
*.pyc
__pycache__/

# Editor files
.vscode/
.idea/
*.swp
*.swo
*~

# OS files
.DS_Store
Thumbs.db

# Sphinx
docs/_build/doctrees/
docs/_build/html/.buildinfo

# Test artifacts
*.gcno
*.gcda
*.gcov
coverage_report.txt
functional_coverage/

# Compiled binaries
*.o
*.so
*.a
FastBuild/

# Credentials (never commit)
.credentials
*.key
*.pem
EOF
    echo -e "${GREEN}✅ Created .gitignore${NC}"
fi

# Add all documentation files
echo ""
echo "Adding documentation files..."
git add .gitignore
git add docs/hld.md docs/systemc_design.md docs/testplan.md
git add docs/conf.py docs/_static/custom.css docs/index.rst
git add doc/Keraunos_PCIe_Tile_HLD.md
git add doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md
git add doc/Keraunos_PCIE_Tile_Testplan.md
git add publish_docs.sh auto_publish_docs.sh
git add README.md 2>/dev/null || true

# Create README if it doesn't exist
if [ ! -f "README.md" ]; then
    cat > README.md << 'EOF'
# Keraunos PCIe Tile

**SystemC/TLM2.0 PCIe Interconnect Bridge**

## Documentation

📚 **[View Online Documentation](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)**

Complete HTML documentation with interactive Mermaid diagrams:
- High-Level Design (16 interactive diagrams)
- SystemC Implementation (~5100 lines)
- Test Plan (76 tests, 100% coverage)

## Features

- **3 Fabric Switches** - NOC-PCIE, NOC-IO, SMN-IO routing
- **8 TLBs** - 5 inbound + 3 outbound address translation
- **MSI-X Relay** - 16-vector interrupt handling
- **System Information Interface** - Configuration tracking
- **Comprehensive Testing** - 76 test cases, 251 assertions

## Quick Start

### View Documentation Locally

```bash
firefox docs/_build/html/index.html
```

### Rebuild Documentation

```bash
cd docs
sphinx-build -b html . _build/html
```

### Publish to GitHub Pages

```bash
./publish_docs.sh
```

## Project Structure

```
.
├── docs/                      # Sphinx documentation
│   ├── hld.md                # High-Level Design
│   ├── systemc_design.md     # SystemC Implementation
│   ├── testplan.md           # Test Plan
│   └── _build/html/          # Generated HTML
├── doc/                       # Source documentation
├── Keraunos_PCIe_tile/       # SystemC source code
│   ├── SystemC/              # Implementation
│   └── Tests/                # Test suite (76 tests)
└── gh-pages/                  # GitHub Pages content

```

## Documentation Build

- **Tool:** Sphinx 5.3.0
- **Diagrams:** Mermaid 10.6.1
- **Theme:** Alabaster with custom CSS
- **Coverage:** 16 interactive diagrams, 100% functional test coverage

## License

Copyright 2026 Tenstorrent Inc.

## Contact

Prasanth Dhanasekar Roy <pdroy@tenstorrent.com>
EOF
    git add README.md
    echo -e "${GREEN}✅ Created README.md${NC}"
fi

# Create initial commit if needed
if ! git rev-parse HEAD &>/dev/null; then
    echo ""
    echo "Creating initial commit..."
    git commit -m "Initial commit: Keraunos PCIe Tile Documentation

Complete SystemC/TLM2.0 documentation with:
- High-Level Design with 16 Mermaid diagrams
- SystemC Implementation (~5100 lines)
- Test Plan with 76 test cases
- Sphinx 5.3.0 + Mermaid 10.6.1 build system

Ready for GitHub Pages deployment."
    echo -e "${GREEN}✅ Initial commit created${NC}"
fi

echo ""
echo "=================================================="
echo "GitHub Repository Creation"
echo "=================================================="
echo ""

# Store credentials temporarily for this session only
echo -e "${YELLOW}Setting up credential helper...${NC}"
git config --global credential.helper 'cache --timeout=3600'

echo ""
echo -e "${BLUE}Please authenticate with GitHub when prompted${NC}"
echo ""

# Check if remote already exists
if git remote get-url origin &>/dev/null; then
    echo "Remote 'origin' already exists: $(git remote get-url origin)"
    EXISTING_URL=$(git remote get-url origin)
    
    if [ "$EXISTING_URL" != "$REPO_URL" ]; then
        echo -e "${YELLOW}Updating remote URL to: ${REPO_URL}${NC}"
        git remote set-url origin "$REPO_URL"
    fi
else
    echo "Adding remote 'origin': ${REPO_URL}"
    git remote add origin "$REPO_URL"
fi

echo ""
echo "Attempting to push to GitHub..."
echo "You will be prompted for username and password/token"
echo ""
echo "Username: pdroy@tenstorrent.com"
echo "Password: (your GitHub password or Personal Access Token)"
echo ""

# Try to push
if git push -u origin main; then
    echo ""
    echo -e "${GREEN}=================================================="
    echo "✅ Repository Successfully Published!"
    echo "==================================================${NC}"
    echo ""
    echo "Repository URL: https://github.com/${GITHUB_USER}/${REPO_NAME}"
    echo ""
    echo -e "${YELLOW}Next Steps:${NC}"
    echo "1. Go to: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
    echo "2. Under 'Build and deployment':"
    echo "   - Source: Deploy from a branch"
    echo "   - Branch: main"
    echo "   - Folder: /gh-pages"
    echo "3. Save and wait 1-2 minutes"
    echo ""
    echo "Then run: ./publish_docs.sh"
    echo ""
else
    echo ""
    echo -e "${RED}Push failed. This usually means:${NC}"
    echo "1. Repository doesn't exist yet on GitHub"
    echo "2. Incorrect credentials"
    echo "3. No permission to push"
    echo ""
    echo -e "${YELLOW}To create the repository:${NC}"
    echo "1. Go to: https://github.com/new"
    echo "2. Repository name: ${REPO_NAME}"
    echo "3. Description: Keraunos PCIe Tile SystemC/TLM2.0 Documentation"
    echo "4. Public repository (recommended for GitHub Pages)"
    echo "5. Click 'Create repository'"
    echo ""
    echo "Then run this script again."
    echo ""
    echo -e "${BLUE}Or use GitHub CLI:${NC}"
    echo "gh repo create ${REPO_NAME} --public --description 'Keraunos PCIe Tile Documentation'"
    echo ""
fi

echo "=================================================="
