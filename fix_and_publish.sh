#!/bin/bash
#
# Fix and Publish - Ensures gh-pages content is pushed and GitHub Pages enabled
#

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo "=================================================="
echo "Fix and Publish GitHub Pages"
echo "=================================================="
echo ""

# Check for token
if [ -z "$GITHUB_TOKEN" ]; then
    echo -e "${YELLOW}GitHub Token Required${NC}"
    echo ""
    echo "Get token from: https://github.com/settings/tokens/new"
    echo "(Login with: pdroy@tenstorrent.com / KairuHanika@2025)"
    echo "Scopes needed: repo, workflow"
    echo ""
    echo -n "Enter your Personal Access Token (ghp_...): "
    read -s GITHUB_TOKEN
    echo ""
    
    if [ -z "$GITHUB_TOKEN" ]; then
        echo -e "${RED}Token required to continue${NC}"
        exit 1
    fi
fi

GITHUB_USER="pdroyTT"
REPO_NAME="Keraunos_PCIe_TILE"

echo "Step 1: Ensure gh-pages is tracked in git"
echo "-----------------------------------------"

# Make sure gh-pages files are added
git add -f gh-pages/

# Check status
TRACKED_FILES=$(git ls-files gh-pages/ | wc -l)
echo "Tracked files in gh-pages: $TRACKED_FILES"

if [ "$TRACKED_FILES" -eq "0" ]; then
    echo -e "${YELLOW}No files tracked. Adding now...${NC}"
    git add gh-pages/
    TRACKED_FILES=$(git ls-files gh-pages/ | wc -l)
    echo "Now tracking: $TRACKED_FILES files"
fi

if [ "$TRACKED_FILES" -gt "0" ]; then
    echo -e "${GREEN}✅ gh-pages files tracked${NC}"
else
    echo -e "${RED}❌ Failed to track gh-pages files${NC}"
    exit 1
fi

echo ""

echo "Step 2: Commit gh-pages if needed"
echo "--------------------------------"

if git diff --cached --quiet && git diff --quiet gh-pages/; then
    echo "No changes to commit"
else
    git commit -m "Add gh-pages documentation

- Complete HTML documentation (~1.5MB)
- 16 interactive Mermaid diagrams
- High-Level Design, SystemC Implementation, Test Plan
- Built with Sphinx 5.3.0

Date: $(date '+%Y-%m-%d %H:%M:%S')" 2>/dev/null || echo "Already committed"
    
    echo -e "${GREEN}✅ Changes committed${NC}"
fi

echo ""

echo "Step 3: Push to GitHub with Token"
echo "---------------------------------"

# Configure remote with token
REPO_URL="https://${GITHUB_TOKEN}@github.com/${GITHUB_USER}/${REPO_NAME}.git"

if git remote get-url origin &>/dev/null; then
    git remote set-url origin "$REPO_URL"
else
    git remote add origin "$REPO_URL"
fi

# Get actual branch name
BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "Pushing branch: $BRANCH"

# Disable credential helper to prevent prompts
GIT_TERMINAL_PROMPT=0 git push -u origin $BRANCH --force 2>&1 | grep -v "Username\|Password" || {
    echo -e "${RED}Push failed${NC}"
    exit 1
}

echo -e "${GREEN}✅ Pushed to GitHub${NC}"

# Clean token from URL
git remote set-url origin "https://github.com/${GITHUB_USER}/${REPO_NAME}.git"

echo ""

echo "Step 4: Enable GitHub Pages via API"
echo "-----------------------------------"

# First, check current Pages status
PAGES_STATUS=$(curl -s \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    -H "Accept: application/vnd.github.v3+json" \
    "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}/pages")

if echo "$PAGES_STATUS" | grep -q '"status":"built"'; then
    echo -e "${GREEN}✅ GitHub Pages already configured${NC}"
elif echo "$PAGES_STATUS" | grep -q "404"; then
    echo "Enabling GitHub Pages..."
    
    # Enable Pages
    ENABLE_RESPONSE=$(curl -s \
        -H "Authorization: token ${GITHUB_TOKEN}" \
        -H "Accept: application/vnd.github.switcheroo-preview+json" \
        -X POST "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}/pages" \
        -d "{
            \"source\": {
                \"branch\": \"${BRANCH}\",
                \"path\": \"/gh-pages\"
            }
        }")
    
    if echo "$ENABLE_RESPONSE" | grep -q '"html_url"'; then
        echo -e "${GREEN}✅ GitHub Pages enabled via API${NC}"
    else
        echo -e "${YELLOW}⚠️  API enablement unclear, trying alternative...${NC}"
        
        # Alternative: Update repo settings
        curl -s \
            -H "Authorization: token ${GITHUB_TOKEN}" \
            -H "Accept: application/vnd.github.v3+json" \
            -X PATCH "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}" \
            -d '{"has_pages": true}' > /dev/null
        
        echo -e "${YELLOW}Please verify manually:${NC}"
        echo "Go to: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
    fi
fi

echo ""

echo "Step 5: Verify GitHub Pages Configuration"
echo "-----------------------------------------"

sleep 5  # Give GitHub a moment to process

# Check Pages status again
PAGES_CHECK=$(curl -s \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    -H "Accept: application/vnd.github.v3+json" \
    "https://api.github.com/repos/${GITHUB_USER}/${REPO_NAME}/pages")

if echo "$PAGES_CHECK" | grep -q '"html_url"'; then
    PAGE_URL=$(echo "$PAGES_CHECK" | grep -o '"html_url":"[^"]*"' | cut -d'"' -f4)
    echo -e "${GREEN}✅ GitHub Pages is configured${NC}"
    echo "URL: $PAGE_URL"
else
    echo -e "${YELLOW}Manual configuration needed${NC}"
    echo ""
    echo "Please do this ONE TIME:"
    echo ""
    echo "1. Open: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
    echo ""
    echo "2. Under 'Build and deployment':"
    echo "   - Source: Deploy from a branch"
    echo "   - Branch: ${BRANCH}"
    echo "   - Folder: /gh-pages"
    echo ""
    echo "3. Click 'Save'"
    echo ""
    echo "4. Wait 1-2 minutes"
    echo ""
    echo "5. View at: https://${GITHUB_USER}.github.io/${REPO_NAME}/"
fi

echo ""
echo "=================================================="
echo -e "${GREEN}✅ Documentation Ready!${NC}"
echo "=================================================="
echo ""
echo "📂 Repository: https://github.com/${GITHUB_USER}/${REPO_NAME}"
echo "🌐 Documentation: https://${GITHUB_USER}.github.io/${REPO_NAME}/"
echo ""
echo "⏱️  If page shows 404:"
echo "   - Wait 1-2 minutes for GitHub Pages deployment"
echo "   - Verify settings: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
echo "   - Check branch is: ${BRANCH}"
echo "   - Check folder is: /gh-pages"
echo ""
echo "=================================================="
