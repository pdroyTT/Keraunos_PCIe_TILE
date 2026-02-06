#!/bin/bash
#
# Auto-Publish Documentation on Changes
# Watches for changes and automatically rebuilds and publishes
#

set -e

echo "=================================================="
echo "Auto-Publish: Keraunos PCIe Tile Documentation"
echo "=================================================="
echo ""
echo "Watching for changes in:"
echo "  - docs/*.md (hld.md, systemc_design.md, testplan.md)"
echo "  - docs/conf.py"
echo "  - docs/_static/custom.css"
echo "  - doc/*.md (source documentation)"
echo ""
echo "Press Ctrl+C to stop"
echo "=================================================="
echo ""

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Function to rebuild and publish
rebuild_and_publish() {
    echo ""
    echo -e "${YELLOW}[$(date '+%H:%M:%S')] Change detected! Rebuilding...${NC}"
    
    # Rebuild HTML
    cd docs
    rm -rf _build/html > /dev/null 2>&1
    sphinx-build -b html . _build/html > /dev/null 2>&1
    BUILD_STATUS=$?
    cd ..
    
    if [ $BUILD_STATUS -ne 0 ]; then
        echo -e "${YELLOW}[$(date '+%H:%M:%S')] ⚠️  Build had warnings, but continuing...${NC}"
    fi
    
    # Create gh-pages if it doesn't exist
    if [ ! -d "gh-pages" ]; then
        mkdir -p gh-pages
    fi
    
    # Update gh-pages
    if [ -d "gh-pages/.git" ]; then
        find gh-pages -mindepth 1 -maxdepth 1 ! -name '.git' -exec rm -rf {} +
    else
        rm -rf gh-pages/*
    fi
    
    cp -r docs/_build/html/* gh-pages/
    touch gh-pages/.nojekyll
    
    # Create updated README
    cat > gh-pages/README.md << 'READMEEOF'
# Keraunos PCIe Tile Documentation

**Professional HTML Documentation with Interactive Diagrams**

## View Online

[View Documentation](https://pdroyTT.github.io/Keraunos_PCIe_TILE/)

## Contents

- **High-Level Design (HLD)** - 16 interactive Mermaid diagrams
- **SystemC Implementation** - Complete low-level design
- **Test Plan** - 76 test cases, 100% coverage

## Last Updated

**Date:** $(date '+%Y-%m-%d %H:%M:%S')  
**Auto-generated:** Yes  
**Build Tool:** Sphinx 5.3.0 + Mermaid 10.6.1

READMEEOF
    
    # Git operations
    if [ ! -d ".git" ]; then
        git init > /dev/null 2>&1
    fi
    
    git add gh-pages/ docs/ doc/ > /dev/null 2>&1
    
    if ! git diff --staged --quiet; then
        git commit -m "Auto-update documentation - $(date '+%Y-%m-%d %H:%M:%S')

Updated: HLD, SystemC Design, Test Plan
Diagrams: 16 Mermaid diagrams
Size: $(du -sh gh-pages 2>/dev/null | cut -f1 || echo 'N/A')" > /dev/null 2>&1
        
        # Try to push if remote exists
        if git remote get-url origin &>/dev/null; then
            BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "main")
            git push origin ${BRANCH} > /dev/null 2>&1 && \
                echo -e "${GREEN}[$(date '+%H:%M:%S')] ✅ Documentation published to GitHub!${NC}" || \
                echo -e "${YELLOW}[$(date '+%H:%M:%S')] ⚠️  Push failed (check network/auth)${NC}"
        else
            echo -e "${BLUE}[$(date '+%H:%M:%S')] ✅ Documentation rebuilt (no remote configured)${NC}"
        fi
        
        echo "View locally: firefox gh-pages/index.html"
    else
        echo -e "${YELLOW}[$(date '+%H:%M:%S')] No changes to publish${NC}"
    fi
    
    echo ""
    echo "Watching for next change..."
}

# Check if inotify-tools is available
if ! command -v inotifywait &> /dev/null; then
    echo -e "${YELLOW}inotify-tools not found. Using polling method (slower)${NC}"
    echo ""
    
    # Polling method (fallback)
    LAST_HASH=$(find docs/ doc/ -name "*.md" -o -name "*.py" -o -name "*.css" -type f -exec md5sum {} \; 2>/dev/null | md5sum)
    
    while true; do
        sleep 5  # Check every 5 seconds
        
        CURRENT_HASH=$(find docs/ doc/ -name "*.md" -o -name "*.py" -o -name "*.css" -type f -exec md5sum {} \; 2>/dev/null | md5sum)
        
        if [ "$CURRENT_HASH" != "$LAST_HASH" ]; then
            rebuild_and_publish
            LAST_HASH=$CURRENT_HASH
        fi
    done
else
    # inotify method (efficient)
    echo -e "${GREEN}Using inotify for efficient file watching${NC}"
    echo ""
    
    while true; do
        # Wait for any relevant file to change
        inotifywait -r -e modify,create,delete \
            --include '\.(md|py|css)$' \
            docs/ doc/ 2>/dev/null
        
        # Debounce - wait 2 seconds for multiple changes
        sleep 2
        
        rebuild_and_publish
    done
fi
