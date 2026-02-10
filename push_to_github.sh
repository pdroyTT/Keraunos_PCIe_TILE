#!/bin/bash
#
# Push to GitHub
# HTML should be pre-built manually with: cd doc && sphinx-build -b html . _build/html
#

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=================================================="
echo " Keraunos PCIe Tile - Push to GitHub"
echo "=================================================="
echo ""

# -------------------------------------------------------
# Step 1: Get GitHub token
# -------------------------------------------------------
echo -e "${BLUE}[1/3] Authenticating with GitHub...${NC}"

if [ -z "$GITHUB_TOKEN" ]; then
    echo -e "${YELLOW}  GitHub Personal Access Token required${NC}"
    echo -n "  Enter token: "
    read -s GITHUB_TOKEN
    echo ""
fi

if [ -z "$GITHUB_TOKEN" ]; then
    echo -e "${RED}  No token provided. Cannot push.${NC}"
    exit 1
fi

# Repository info
GITHUB_USER="pdroyTT"
REPO_NAME="Keraunos_PCIe_TILE"
REPO_URL="https://${GITHUB_TOKEN}@github.com/${GITHUB_USER}/${REPO_NAME}.git"

echo -e "${GREEN}  Token received${NC}"

# -------------------------------------------------------
# Step 2: Stage and commit changes
# -------------------------------------------------------
echo ""
echo -e "${BLUE}[2/3] Staging and committing changes...${NC}"

# Show what will be committed
CHANGED=$(git status --porcelain | wc -l)
if [ "$CHANGED" -eq 0 ]; then
    echo -e "${YELLOW}  No changes to commit${NC}"
else
    echo -e "  ${CHANGED} files changed"

    # Stage key directories (excluding .md files via .gitignore)
    git add -A \
        docs/ \
        src/ \
        include/ \
        tb/ \
        Keraunos_PCIe_tile/ \
        Makefile* \
        .gitignore \
        push_to_github.sh \
        2>/dev/null || true

    # Show staged summary
    STAGED_COUNT=$(git diff --cached --name-only | wc -l)
    if [ "$STAGED_COUNT" -eq 0 ]; then
        echo -e "${YELLOW}  No changes staged (all filtered by .gitignore)${NC}"
    else
        STAGED_SUMMARY=$(git diff --cached --stat | tail -1)
        echo -e "  Staged: ${STAGED_SUMMARY}"

        # Commit with descriptive message
        TIMESTAMP=$(date '+%Y-%m-%d %H:%M')
        
        # Detect what changed
        if git diff --cached --name-only | grep -q '^docs/'; then
            COMMIT_MSG="Update documentation (HTML) - ${TIMESTAMP}"
        elif git diff --cached --name-only | grep -qE '^\(src/|include/|Keraunos_PCIe_tile/SystemC/\)'; then
            COMMIT_MSG="Update source code - ${TIMESTAMP}"
        elif git diff --cached --name-only | grep -q 'Test'; then
            COMMIT_MSG="Update tests - ${TIMESTAMP}"
        else
            COMMIT_MSG="Update project files - ${TIMESTAMP}"
        fi
        
        git commit -m "${COMMIT_MSG}" 2>&1 || echo -e "${YELLOW}  Nothing to commit${NC}"
    fi
fi

# -------------------------------------------------------
# Step 3: Push to GitHub
# -------------------------------------------------------
echo ""
echo -e "${BLUE}[3/3] Pushing to GitHub...${NC}"

# Temporarily set remote with token
git remote set-url origin "$REPO_URL" 2>/dev/null || git remote add origin "$REPO_URL"

BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo -e "  Branch: ${BRANCH}"

GIT_TERMINAL_PROMPT=0 git push origin "$BRANCH" 2>&1 | grep -v "Username\|Password" && {
    echo -e "${GREEN}  Push succeeded${NC}"
} || {
    echo ""
    echo -e "${YELLOW}  Regular push failed. Attempting force push...${NC}"
    echo -n "  Force push to ${BRANCH}? (y/n) "
    read -r REPLY
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        GIT_TERMINAL_PROMPT=0 git push -f origin "$BRANCH" 2>&1 | grep -v "Username\|Password"
        echo -e "${GREEN}  Force push succeeded${NC}"
    else
        echo -e "${RED}  Push aborted${NC}"
        # Clean token from remote URL (security)
        git remote set-url origin "https://github.com/${GITHUB_USER}/${REPO_NAME}.git"
        exit 1
    fi
}

# Clean token from remote URL (security)
git remote set-url origin "https://github.com/${GITHUB_USER}/${REPO_NAME}.git"

# -------------------------------------------------------
# Done
# -------------------------------------------------------
echo ""
echo "=================================================="
echo -e "${GREEN} Push complete!${NC}"
echo "=================================================="
echo ""
echo "Repository: https://github.com/${GITHUB_USER}/${REPO_NAME}"
echo ""
echo -e "${BLUE}GitHub Pages Setup:${NC}"
echo "  1. Go to: https://github.com/${GITHUB_USER}/${REPO_NAME}/settings/pages"
echo "  2. Source: Deploy from a branch"
echo "  3. Branch: ${BRANCH} / Folder: /docs"
echo "  4. Click Save, wait 1-2 minutes"
echo "  5. View: https://${GITHUB_USER,,}.github.io/${REPO_NAME}/"
echo ""
echo -e "${YELLOW}Note: To rebuild HTML documentation, run:${NC}"
echo "  cd doc && sphinx-build -b html . _build/html"
echo "  rsync -a --delete --exclude='.doctrees' --exclude='.buildinfo' _build/html/ ../docs/"
echo ""
echo "=================================================="
