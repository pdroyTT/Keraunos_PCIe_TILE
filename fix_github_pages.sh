#!/bin/bash
#
# Fix GitHub Pages - Move HTML to /docs instead of /gh-pages
# GitHub Pages only recognizes /docs or / (root), not /gh-pages
#

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "=================================================="
echo "Fix GitHub Pages Structure"
echo "=================================================="
echo ""

echo -e "${YELLOW}Issue: GitHub Pages doesn't support /gh-pages folder${NC}"
echo -e "${BLUE}Solution: Moving HTML to /docs folder${NC}"
echo ""

# Step 1: Backup current docs source
echo "Step 1: Backup Sphinx source files"
echo "-----------------------------------"

mkdir -p docs_source
cp -r docs/*.md docs/*.rst docs/*.py docs/_static docs_source/ 2>/dev/null || true
echo -e "${GREEN}✅ Source files backed up to docs_source/${NC}"
echo ""

# Step 2: Clear docs and copy HTML
echo "Step 2: Move HTML to /docs"
echo "-------------------------"

# Remove everything from docs except _build
rm -rf docs/* 2>/dev/null || true
mkdir -p docs

# Copy all HTML from gh-pages to docs
cp -r gh-pages/* docs/

echo -e "${GREEN}✅ HTML documentation moved to /docs${NC}"
echo ""

# Step 3: Update git
echo "Step 3: Commit changes"
echo "---------------------"

git add docs/
git add docs_source/

# Remove gh-pages from tracking
git rm -rf gh-pages/ 2>/dev/null || true

git commit -m "Fix GitHub Pages: Move HTML to /docs folder

GitHub Pages requires /docs or / folder, not /gh-pages.

Changes:
- Moved all HTML from gh-pages/ to docs/
- Backed up Sphinx source to docs_source/
- Ready for GitHub Pages deployment from /docs

Documentation includes:
- index.html, hld.html, systemc_design.html, testplan.html
- 16 interactive Mermaid diagrams
- All static assets and search functionality"

echo -e "${GREEN}✅ Changes committed${NC}"
echo ""

# Step 4: Push to GitHub
echo "Step 4: Push to GitHub"
echo "---------------------"

# Check for token
if [ -z "$GITHUB_TOKEN" ]; then
    echo -e "${YELLOW}GitHub Token needed for push${NC}"
    echo -n "Enter token (or press Enter to skip): "
    read -s GITHUB_TOKEN
    echo ""
fi

if [ -n "$GITHUB_TOKEN" ]; then
    GITHUB_USER="pdroyTT"
    REPO_NAME="Keraunos_PCIe_TILE"
    
    # Configure remote with token
    REPO_URL="https://${GITHUB_TOKEN}@github.com/${GITHUB_USER}/${REPO_NAME}.git"
    git remote set-url origin "$REPO_URL" 2>/dev/null || git remote add origin "$REPO_URL"
    
    # Push
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    GIT_TERMINAL_PROMPT=0 git push origin $BRANCH 2>&1 | grep -v "Username\|Password"
    
    # Clean token
    git remote set-url origin "https://github.com/${GITHUB_USER}/${REPO_NAME}.git"
    
    echo -e "${GREEN}✅ Pushed to GitHub${NC}"
else
    echo -e "${YELLOW}Skipped push (no token provided)${NC}"
    echo "Run: git push origin master"
fi

echo ""
echo "=================================================="
echo -e "${GREEN}✅ GitHub Pages Structure Fixed!${NC}"
echo "=================================================="
echo ""
echo "Next steps:"
echo ""
echo "1. Go to: https://github.com/pdroyTT/Keraunos_PCIe_TILE/settings/pages"
echo ""
echo "2. Configure:"
echo "   - Source: Deploy from a branch"
echo "   - Branch: master"
echo "   - Folder: /docs  ← SELECT THIS (not /gh-pages)"
echo ""
echo "3. Click Save"
echo ""
echo "4. Wait 1-2 minutes"
echo ""
echo "5. View at: https://pdroytt.github.io/Keraunos_PCIe_TILE/"
echo ""
echo "=================================================="
echo ""
echo "📁 Sphinx source files moved to: docs_source/"
echo "📄 HTML documentation now in: docs/"
echo "🌐 GitHub Pages will serve from: /docs"
echo ""
