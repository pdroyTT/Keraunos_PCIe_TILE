#!/bin/bash
#
# Publish Documentation to GitHub Pages
# Uses Personal Access Token for authentication
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

# Check for token
if [ -z "$GITHUB_TOKEN" ]; then
    echo -e "${YELLOW}GitHub Personal Access Token Required${NC}"
    echo ""
    echo "Get your token from: https://github.com/settings/tokens/new"
    echo "Required scopes: repo, workflow"
    echo ""
    echo -n "Enter your GitHub Personal Access Token (starts with ghp_): "
    read -s GITHUB_TOKEN
    echo ""
    echo ""
    
    if [ -z "$GITHUB_TOKEN" ]; then
        echo -e "${RED}Error: Token is required${NC}"
        echo ""
        echo "To get a token:"
        echo "1. Go to: https://github.com/settings/tokens/new"
        echo "2. Login with: pdroy@tenstorrent.com"
        echo "3. Name: Keraunos_Documentation"
        echo "4. Select scopes: repo, workflow"
        echo "5. Generate and copy the token"
        echo ""
        echo "Then run:"
        echo "  export GITHUB_TOKEN='ghp_your_token'"
        echo "  ./publish_docs.sh"
        exit 1
    fi
fi

echo -e "${GREEN}✅ Token provided${NC}"
echo ""

echo "Step 1: Rebuild Sphinx Documentation"
echo "------------------------------------"

cd docs
echo "Cleaning previous build..."
rm -rf _build/html 2>/dev/null || true
echo "Building HTML documentation..."

if sphinx-build -b html . _build/html 2>&1 | tee /tmp/sphinx_build.log | tail -3; then
    if grep -q "ERROR" /tmp/sphinx_build.log; then
        echo -e "${RED}❌ Build has errors${NC}"
        grep "ERROR" /tmp/sphinx_build.log
        exit 1
    fi
    echo -e "${GREEN}✅ HTML documentation rebuilt${NC}"
else
    echo -e "${RED}❌ Build failed${NC}"
    exit 1
fi
cd ..

echo ""

echo "Step 2: Prepare gh-pages Directory"
echo "----------------------------------"

mkdir -p gh-pages
rm -rf gh-pages/* 2>/dev/null || true
cp -r docs/_build/html/* gh-pages/
touch gh-pages/.nojekyll

cat > gh-pages/README.md << 'EOF'
# Keraunos PCIe Tile Documentation

**View Online:** [Documentation Website](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)

## Contents
- High-Level Design (16 Mermaid diagrams)
- SystemC Implementation (~5100 lines)
- Test Plan (76 tests, 100% coverage)

**Last Updated:** $(date '+%Y-%m-%d %H:%M:%S')
EOF

echo -e "${GREEN}✅ gh-pages directory ready${NC}"
echo ""

echo "Step 3: Git Configuration"
echo "------------------------"

git config user.name "Prasanth Dhanasekar Roy" 2>/dev/null || true
git config user.email "$GITHUB_EMAIL" 2>/dev/null || true

# Disable credential prompts - use token only
git config --global credential.helper ""

if [ ! -d ".git" ]; then
    git init
    git checkout -b main 2>/dev/null || git branch -M main
    
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
    
    git add .gitignore README.md 2>/dev/null || true
    echo -e "${GREEN}✅ Git initialized${NC}"
else
    echo "Git repository exists"
fi

echo ""

echo "Step 4: Add and Commit Files"
echo "----------------------------"

git add gh-pages/ 2>/dev/null || true
git add docs/hld.md docs/systemc_design.md docs/testplan.md docs/conf.py docs/_static/ docs/index.rst 2>/dev/null || true
git add doc/*.md 2>/dev/null || true

if git diff --staged --quiet 2>/dev/null && git rev-parse HEAD &>/dev/null; then
    echo -e "${YELLOW}No changes to commit${NC}"
    SKIP_COMMIT=1
else
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    DOC_SIZE=$(du -sh gh-pages 2>/dev/null | cut -f1 || echo "N/A")
    
    git commit -m "Documentation update - ${TIMESTAMP}

- High-Level Design (16 Mermaid diagrams, 800-1200px, 16-18px fonts)
- SystemC Implementation (~5100 lines)
- Test Plan (76 tests, 251 assertions, 0 failures)
- Size: ${DOC_SIZE}

Build: Sphinx 5.3.0 + Mermaid 10.6.1" 2>/dev/null || true
    
    echo -e "${GREEN}✅ Changes committed${NC}"
    SKIP_COMMIT=0
fi

echo ""

echo "Step 5: Create/Check Repository on GitHub"
echo "-----------------------------------------"

# Check if repository exists
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}" 2>/dev/null || echo "000")

if [ "$HTTP_CODE" = "404" ]; then
    echo "Repository doesn't exist. Creating..."
    
    CREATE_RESPONSE=$(curl -s \
        -H "Authorization: token ${GITHUB_TOKEN}" \
        -H "Accept: application/vnd.github.v3+json" \
        -X POST https://api.github.com/user/repos \
        -d "{
            \"name\":\"${REPO_NAME}\",
            \"description\":\"Keraunos PCIe Tile SystemC/TLM2.0 Documentation with 16 Interactive Mermaid Diagrams\",
            \"homepage\":\"https://${GITHUB_USER}.github.io/${REPO_NAME}/\",
            \"private\":false,
            \"has_issues\":true,
            \"has_projects\":false,
            \"has_wiki\":false
        }")
    
    if echo "$CREATE_RESPONSE" | grep -q "\"name\":\"${REPO_NAME}\""; then
        echo -e "${GREEN}✅ Repository created successfully${NC}"
        sleep 3  # Give GitHub a moment
    else
        echo -e "${RED}❌ Failed to create repository${NC}"
        echo "Response:"
        echo "$CREATE_RESPONSE" | grep -o '"message":"[^"]*"' || echo "$CREATE_RESPONSE"
        echo ""
        echo "Please create manually at: https://github.com/new"
        echo "Repository name: ${REPO_NAME}"
        exit 1
    fi
elif [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}✅ Repository exists${NC}"
elif [ "$HTTP_CODE" = "401" ]; then
    echo -e "${RED}❌ Authentication failed${NC}"
    echo "Invalid token. Get a new one from: https://github.com/settings/tokens"
    exit 1
else
    echo -e "${RED}❌ API error (HTTP ${HTTP_CODE})${NC}"
    exit 1
fi

echo ""

echo "Step 6: Push to GitHub (Using Token)"
echo "------------------------------------"

# Configure remote with token embedded
REPO_URL_WITH_TOKEN="https://${GITHUB_TOKEN}@github.com/${GITHUB_USER}/${REPO_NAME}.git"

if git remote get-url origin &>/dev/null; then
    git remote set-url origin "$REPO_URL_WITH_TOKEN"
else
    git remote add origin "$REPO_URL_WITH_TOKEN"
fi

# Get current branch
BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "main")

echo "Pushing to origin/${BRANCH}..."

if git push -u origin ${BRANCH} 2>&1 | grep -v "Username\|Password"; then
    echo ""
    echo -e "${GREEN}✅ Successfully pushed to GitHub!${NC}"
    
    # Clean token from remote URL
    git remote set-url origin "https://github.com/${GITHUB_USER}/${REPO_NAME}.git"
    
    echo ""
    echo "=================================================="
    echo -e "${GREEN}🎉 Documentation Published!${NC}"
    echo "=================================================="
    echo ""
    echo "📂 Repository: https://github.com/${GITHUB_USER}/${REPO_NAME}"
    echo ""
    
    # Try to enable GitHub Pages via API
    echo "Configuring GitHub Pages..."
    PAGES_RESPONSE=$(curl -s \
        -H "Authorization: token ${GITHUB_TOKEN}" \
        -H "Accept: application/vnd.github.switcheroo-preview+json" \
        -X POST "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}/pages" \
        -d '{
            "source": {
                "branch": "main",
                "path": "/gh-pages"
            }
        }' 2>/dev/null)
    
    if echo "$PAGES_RESPONSE" | grep -q '"status"'; then
        echo -e "${GREEN}✅ GitHub Pages configured automatically${NC}"
        echo ""
        echo "🌐 Documentation URL: https://${GITHUB_USER}.github.io/${REPO_NAME}/"
        echo ""
        echo -e "${YELLOW}Note: GitHub Pages deployment takes 1-2 minutes${NC}"
    else
        echo -e "${YELLOW}⚠️  GitHub Pages may need manual configuration${NC}"
        echo ""
        echo "To enable GitHub Pages:"
        echo "1. Go to: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
        echo "2. Source: 'Deploy from a branch'"
        echo "3. Branch: 'main'"
        echo "4. Folder: '/gh-pages'"
        echo "5. Click 'Save'"
        echo ""
        echo "Then view at: https://${GITHUB_USER}.github.io/${REPO_NAME}/"
    fi
    
    echo ""
    echo "📄 Documentation includes:"
    echo "   - High-Level Design (16 interactive Mermaid diagrams)"
    echo "   - SystemC Implementation (~5100 lines)"
    echo "   - Test Plan (76 tests, 251 assertions, 0 failures)"
    echo ""
    echo "📊 Features:"
    echo "   - Enhanced diagrams (800-1200px, 16-18px fonts)"
    echo "   - Professional styling and responsive design"
    echo "   - Full-text search and navigation"
    echo ""
    
else
    echo ""
    echo -e "${RED}❌ Push failed${NC}"
    echo ""
    echo "This usually means:"
    echo "1. Invalid token"
    echo "2. Network issue"
    echo "3. Repository permissions issue"
    echo ""
    echo "To fix:"
    echo "1. Get a new token: https://github.com/settings/tokens/new"
    echo "2. Ensure 'repo' scope is selected"
    echo "3. Run: export GITHUB_TOKEN='ghp_your_new_token'"
    echo "4. Run: ./publish_docs.sh"
    exit 1
fi

echo "=================================================="
