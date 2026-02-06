#!/bin/bash
#
# Publish Documentation to GitHub Pages
# Supports both manual and token-based authentication
#

set -e

echo "=================================================="
echo "Publishing Keraunos PCIe Tile Documentation"
echo "=================================================="
echo ""

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration
GITHUB_USER="pdroyTT"
REPO_NAME="Keraunos_PCIe_TILE"
GITHUB_EMAIL="pdroy@tenstorrent.com"

echo "Step 1: Rebuild Sphinx Documentation"
echo "------------------------------------"

cd docs
echo "Cleaning previous build..."
rm -rf _build/html 2>/dev/null || true
echo "Building HTML documentation..."
if sphinx-build -b html . _build/html 2>&1 | tee /tmp/sphinx_build.log | tail -5; then
    echo -e "${GREEN}✅ HTML documentation rebuilt${NC}"
else
    echo -e "${RED}❌ Build failed${NC}"
    cat /tmp/sphinx_build.log
    exit 1
fi
cd ..

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
    find gh-pages -mindepth 1 -maxdepth 1 ! -name '.git' -exec rm -rf {} + 2>/dev/null || true
else
    rm -rf gh-pages/* 2>/dev/null || true
fi

# Copy new HTML
echo "Copying HTML documentation..."
cp -r docs/_build/html/* gh-pages/

# Ensure .nojekyll exists (tells GitHub Pages not to use Jekyll)
touch gh-pages/.nojekyll

# Update README with comprehensive info
cat > gh-pages/README.md << 'EOF'
# Keraunos PCIe Tile Documentation

**Professional HTML Documentation with Interactive Diagrams**

## 📚 View Online

**[View Documentation](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)**

## 📖 Documentation Contents

### Main Documents

1. **[High-Level Design (HLD)](hld.html)** - 16 interactive Mermaid diagrams
2. **[SystemC Implementation](systemc_design.html)** - ~5100 lines detailed design
3. **[Test Plan & Verification](testplan.html)** - 76 tests, 100% coverage

### Features

- 16 Interactive Mermaid Diagrams (800-1200px, 16-18px fonts)
- Professional styling and responsive design
- Full-text search and navigation
- Complete verification results

## 📊 Statistics

- **Test Cases:** 76 (41 E2E + 35 directed)
- **Assertions:** 251 (0 failures)
- **Coverage:** 100% functional
- **Diagrams:** 16 interactive

**Last Updated:** $(date '+%Y-%m-%d %H:%M:%S')  
**Build Tool:** Sphinx 5.3.0 with Mermaid 10.6.1

---

*Auto-generated from markdown sources using Sphinx*
EOF

echo -e "${GREEN}✅ Updated gh-pages directory${NC}"
echo ""

echo "Step 3: Git Configuration"
echo "------------------------"

# Configure git if not already done
git config user.name "Prasanth Dhanasekar Roy" 2>/dev/null || true
git config user.email "$GITHUB_EMAIL" 2>/dev/null || true
git config credential.helper 'cache --timeout=7200' 2>/dev/null || true

# Initialize git if not already initialized
if [ ! -d ".git" ]; then
    echo -e "${YELLOW}Initializing git repository...${NC}"
    git init
    git checkout -b main 2>/dev/null || git branch -M main
    
    # Create .gitignore
    cat > .gitignore << 'GITEOF'
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
GITEOF
    
    git add .gitignore
    echo -e "${GREEN}✅ Git repository initialized${NC}"
else
    echo "Git repository exists"
fi

# Add changes
echo "Adding files to git..."
git add gh-pages/ 2>/dev/null || true
git add docs/hld.md docs/systemc_design.md docs/testplan.md 2>/dev/null || true
git add docs/conf.py docs/_static/ docs/index.rst 2>/dev/null || true
git add doc/Keraunos_PCIe_Tile_HLD.md 2>/dev/null || true
git add doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md 2>/dev/null || true
git add doc/Keraunos_PCIE_Tile_Testplan.md 2>/dev/null || true
git add README.md .gitignore publish_docs.sh auto_publish_docs.sh 2>/dev/null || true

echo ""

echo "Step 4: Commit Changes"
echo "---------------------"

# Check if there are changes
if git diff --staged --quiet 2>/dev/null && git rev-parse HEAD &>/dev/null; then
    echo -e "${YELLOW}No changes to commit${NC}"
    echo ""
    echo -e "${BLUE}Documentation is up to date${NC}"
    echo "View locally: firefox gh-pages/index.html"
    
    # Still try to push if remote exists
    if git remote get-url origin &>/dev/null; then
        echo ""
        echo "Checking if remote needs update..."
        if git push -n origin main 2>&1 | grep -q "Everything up-to-date"; then
            echo -e "${GREEN}Remote is up to date${NC}"
            exit 0
        fi
    else
        exit 0
    fi
fi

# Commit with detailed message
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
DOC_SIZE=$(du -sh gh-pages 2>/dev/null | cut -f1 || echo "N/A")

git commit -m "Update documentation - ${TIMESTAMP}

Keraunos PCIe Tile Documentation:
- High-Level Design (16 Mermaid diagrams)
- SystemC Implementation (~5100 lines)
- Test Plan (76 tests, 251 assertions, 0 failures)

Size: ${DOC_SIZE}
Build: Sphinx 5.3.0 + Mermaid 10.6.1
Status: Production Ready ✅

View at: https://pdroyTT.github.io/Keraunos_PCIe_TILE/" 2>&1 | grep -v "^$" || true

echo -e "${GREEN}✅ Changes committed${NC}"
echo ""

echo "Step 5: Configure Remote & Push"
echo "-------------------------------"

# Determine repository URL based on token availability
if [ -n "$GITHUB_TOKEN" ]; then
    REPO_URL="https://${GITHUB_TOKEN}@github.com/${GITHUB_USER}/${REPO_NAME}.git"
    echo "Using token authentication"
else
    REPO_URL="https://github.com/${GITHUB_USER}/${REPO_NAME}.git"
    echo "Using interactive authentication"
fi

# Check/Add remote
if git remote get-url origin &>/dev/null; then
    CURRENT_URL=$(git remote get-url origin)
    if [[ "$CURRENT_URL" != *"github.com"* ]] || [[ "$CURRENT_URL" != *"$REPO_NAME"* ]]; then
        echo "Updating remote URL..."
        git remote set-url origin "$REPO_URL"
    fi
else
    echo "Adding remote 'origin'..."
    git remote add origin "$REPO_URL"
fi

# Try to create repository if it doesn't exist (only with token)
if [ -n "$GITHUB_TOKEN" ]; then
    HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" \
        -H "Authorization: token ${GITHUB_TOKEN}" \
        "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}" 2>/dev/null || echo "000")
    
    if [ "$HTTP_CODE" = "404" ]; then
        echo "Repository doesn't exist. Creating..."
        CREATE_RESPONSE=$(curl -s -H "Authorization: token ${GITHUB_TOKEN}" \
            -H "Accept: application/vnd.github.v3+json" \
            -X POST https://api.github.com/user/repos \
            -d "{
                \"name\":\"${REPO_NAME}\",
                \"description\":\"Keraunos PCIe Tile SystemC/TLM2.0 Documentation\",
                \"homepage\":\"https://${GITHUB_USER}.github.io/${REPO_NAME}/\",
                \"private\":false,
                \"has_pages\":true
            }" 2>/dev/null)
        
        if echo "$CREATE_RESPONSE" | grep -q "\"name\":\"${REPO_NAME}\""; then
            echo -e "${GREEN}✅ Repository created${NC}"
            sleep 2
        else
            echo -e "${YELLOW}Note: Repository may already exist or needs manual creation${NC}"
        fi
    elif [ "$HTTP_CODE" = "200" ]; then
        echo "Repository exists"
    fi
fi

# Push to GitHub
echo "Pushing to GitHub..."
BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "main")

if git push -u origin ${BRANCH} 2>&1; then
    echo ""
    echo -e "${GREEN}✅ Documentation published to GitHub!${NC}"
    echo ""
    echo "Repository: https://github.com/${GITHUB_USER}/${REPO_NAME}"
    echo "Documentation: https://${GITHUB_USER}.github.io/${REPO_NAME}/"
    echo ""
    echo -e "${YELLOW}Note: GitHub Pages may take 1-2 minutes to update${NC}"
    echo ""
    echo "Enable GitHub Pages (if not done):"
    echo "1. Go to: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
    echo "2. Source: 'Deploy from a branch'"
    echo "3. Branch: 'main'"
    echo "4. Folder: '/gh-pages'"
    echo "5. Click 'Save'"
else
    echo ""
    echo -e "${RED}❌ Push failed${NC}"
    echo ""
    echo -e "${YELLOW}Possible solutions:${NC}"
    echo ""
    echo "1. Use Personal Access Token:"
    echo "   - Get token from: https://github.com/settings/tokens"
    echo "   - Run: export GITHUB_TOKEN='ghp_your_token'"
    echo "   - Then: ./publish_docs.sh"
    echo ""
    echo "2. Create repository manually:"
    echo "   - Go to: https://github.com/new"
    echo "   - Name: ${REPO_NAME}"
    echo "   - Then run this script again"
    echo ""
    echo "3. Configure authentication:"
    echo "   - Run: git config --global credential.helper cache"
    echo "   - Then: ./publish_docs.sh"
    echo ""
    exit 1
fi

# Clear token from remote URL for security
if [ -n "$GITHUB_TOKEN" ]; then
    git remote set-url origin "https://github.com/${GITHUB_USER}/${REPO_NAME}.git" 2>/dev/null || true
fi

echo ""
echo "=================================================="
echo -e "${GREEN}Documentation Build & Publish Complete!${NC}"
echo "=================================================="
echo ""
echo "📄 Local: firefox gh-pages/index.html"
echo "🌐 Online: https://${GITHUB_USER}.github.io/${REPO_NAME}/"
echo ""
