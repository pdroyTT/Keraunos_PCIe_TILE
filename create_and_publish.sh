#!/bin/bash
#
# Create GitHub Repository and Publish Documentation
# Automated setup with credentials
#

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo "=================================================="
echo "Keraunos PCIe Tile - GitHub Setup & Publish"
echo "=================================================="
echo ""

# Configuration
GITHUB_USER="pdroyTT"
REPO_NAME="Keraunos_PCIe_TILE"
GITHUB_EMAIL="pdroy@tenstorrent.com"
REPO_URL="https://${GITHUB_USER}:${GITHUB_PASSWORD}@github.com/${GITHUB_USER}/${REPO_NAME}.git"
API_URL="https://api.github.com/user/repos"

# Read credentials securely
if [ -z "$GITHUB_PASSWORD" ]; then
    echo "Enter GitHub password or Personal Access Token:"
    read -s GITHUB_PASSWORD
    echo ""
fi

echo "Step 1: Configure Git"
echo "--------------------"
git config --global user.name "Prasanth Dhanasekar Roy"
git config --global user.email "$GITHUB_EMAIL"
git config --global credential.helper 'cache --timeout=7200'
echo -e "${GREEN}✅ Git configured${NC}"
echo ""

echo "Step 2: Initialize Local Repository"
echo "-----------------------------------"

# Initialize git if needed
if [ ! -d ".git" ]; then
    git init
    git checkout -b main 2>/dev/null || git branch -M main
    echo -e "${GREEN}✅ Repository initialized${NC}"
fi

# Create .gitignore
cat > .gitignore << 'EOF'
docs/_build/doctrees/
*.pyc
__pycache__/
.vscode/
*.swp
.DS_Store
*.gcno
*.gcda
*.o
*.so
FastBuild/
EOF

# Create README
cat > README.md << 'EOF'
# Keraunos PCIe Tile

**SystemC/TLM2.0 PCIe Interconnect Bridge**

## 📚 Documentation

**[View Online Documentation](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)**

Professional HTML documentation with interactive Mermaid diagrams

### Contents
- **High-Level Design** - 16 interactive diagrams
- **SystemC Implementation** - ~5100 lines of detailed design
- **Test Plan** - 76 tests, 251 assertions, 100% coverage

## Features

- 3 Fabric Switches (NOC-PCIE, NOC-IO, SMN-IO)
- 8 TLBs (5 inbound + 3 outbound)
- MSI-X Relay (16 vectors)
- System Information Interface (SII)
- Comprehensive test suite

## Build

```bash
cd docs
sphinx-build -b html . _build/html
```

## Publish

```bash
./publish_docs.sh
```

---
Copyright 2026 Tenstorrent Inc.
EOF

# Add files
git add .gitignore README.md publish_docs.sh auto_publish_docs.sh
git add docs/hld.md docs/systemc_design.md docs/testplan.md docs/conf.py docs/_static/ docs/index.rst 2>/dev/null || true
git add doc/*.md 2>/dev/null || true

# Commit if needed
if ! git rev-parse HEAD &>/dev/null || ! git diff --staged --quiet; then
    git commit -m "Initial commit: Keraunos PCIe Tile Documentation

Complete SystemC/TLM2.0 documentation:
- High-Level Design (16 Mermaid diagrams)
- SystemC Implementation
- Test Plan (76 tests)
- Sphinx 5.3.0 + Mermaid 10.6.1" || true
    echo -e "${GREEN}✅ Files committed${NC}"
fi
echo ""

echo "Step 3: Check/Create GitHub Repository"
echo "--------------------------------------"

# Check if repo exists
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" \
    -u "${GITHUB_EMAIL}:${GITHUB_PASSWORD}" \
    "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}")

if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${BLUE}Repository already exists${NC}"
elif [ "$HTTP_CODE" = "404" ]; then
    echo "Creating repository on GitHub..."
    
    RESPONSE=$(curl -s -u "${GITHUB_EMAIL}:${GITHUB_PASSWORD}" \
        -X POST "$API_URL" \
        -d "{\"name\":\"${REPO_NAME}\",\"description\":\"Keraunos PCIe Tile SystemC/TLM2.0 Documentation with Interactive Diagrams\",\"homepage\":\"https://${GITHUB_USER}.github.io/${REPO_NAME}/\",\"private\":false,\"has_issues\":true,\"has_projects\":false,\"has_wiki\":false}")
    
    if echo "$RESPONSE" | grep -q "\"name\":\"${REPO_NAME}\""; then
        echo -e "${GREEN}✅ Repository created successfully${NC}"
    else
        echo -e "${RED}Failed to create repository${NC}"
        echo "Response: $RESPONSE"
        exit 1
    fi
else
    echo -e "${RED}Authentication failed (HTTP $HTTP_CODE)${NC}"
    echo "Check your credentials"
    exit 1
fi
echo ""

echo "Step 4: Build Documentation"
echo "--------------------------"
cd docs
rm -rf _build/html
sphinx-build -b html . _build/html 2>&1 | tail -5
cd ..
echo -e "${GREEN}✅ Documentation built${NC}"
echo ""

echo "Step 5: Setup gh-pages"
echo "---------------------"
mkdir -p gh-pages
if [ -d "gh-pages/.git" ]; then
    find gh-pages -mindepth 1 -maxdepth 1 ! -name '.git' -exec rm -rf {} +
else
    rm -rf gh-pages/*
fi

cp -r docs/_build/html/* gh-pages/
touch gh-pages/.nojekyll

cat > gh-pages/README.md << 'EOF'
# Keraunos PCIe Tile Documentation

**View Online:** [https://pdroyTT.github.io/Keraunos_PCIe_TILE/](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)

Auto-generated HTML documentation with 16 interactive Mermaid diagrams.

**Last Updated:** $(date '+%Y-%m-%d %H:%M:%S')
EOF

git add gh-pages/
echo -e "${GREEN}✅ gh-pages prepared${NC}"
echo ""

echo "Step 6: Push to GitHub"
echo "---------------------"

# Set remote
if git remote get-url origin &>/dev/null; then
    git remote set-url origin "https://${GITHUB_USER}:${GITHUB_PASSWORD}@github.com/${GITHUB_USER}/${REPO_NAME}.git"
else
    git remote add origin "https://${GITHUB_USER}:${GITHUB_PASSWORD}@github.com/${GITHUB_USER}/${REPO_NAME}.git"
fi

# Push
git push -u origin main --force

echo -e "${GREEN}✅ Pushed to GitHub${NC}"
echo ""

echo "=================================================="
echo -e "${GREEN}✅ SUCCESS!${NC}"
echo "=================================================="
echo ""
echo "Repository: https://github.com/${GITHUB_USER}/${REPO_NAME}"
echo ""
echo -e "${YELLOW}Enable GitHub Pages:${NC}"
echo "1. Go to: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
echo "2. Source: 'Deploy from a branch'"
echo "3. Branch: 'main'"
echo "4. Folder: '/gh-pages'"
echo "5. Click 'Save'"
echo ""
echo "Documentation will be available at:"
echo "https://${GITHUB_USER}.github.io/${REPO_NAME}/"
echo ""
echo "Wait 1-2 minutes for GitHub Pages to deploy"
echo "=================================================="
