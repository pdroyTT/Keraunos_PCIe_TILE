#!/bin/bash
#
# Helper script to run publish_docs.sh with token
# This avoids any username/password prompts
#

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo "=================================================="
echo "Keraunos PCIe Tile - Publish Helper"
echo "=================================================="
echo ""

# Check if token is already set
if [ -n "$GITHUB_TOKEN" ]; then
    echo -e "${GREEN}✅ Token found in environment${NC}"
    echo ""
else
    echo -e "${YELLOW}GitHub Personal Access Token Setup${NC}"
    echo ""
    echo "To get your token:"
    echo "1. Go to: https://github.com/settings/tokens/new"
    echo "2. Login with: pdroy@tenstorrent.com / KairuHanika@2025"
    echo "3. Token name: Keraunos_Documentation"
    echo "4. Expiration: 90 days"
    echo "5. Select scopes:"
    echo "   ✅ repo (all sub-options)"
    echo "   ✅ workflow"
    echo "6. Click 'Generate token'"
    echo "7. Copy the token (starts with ghp_)"
    echo ""
    echo -e "${BLUE}Enter your Personal Access Token:${NC}"
    read -s GITHUB_TOKEN
    echo ""
    
    if [ -z "$GITHUB_TOKEN" ]; then
        echo -e "${RED}No token provided. Exiting.${NC}"
        exit 1
    fi
    
    # Export for the publish script
    export GITHUB_TOKEN
    echo -e "${GREEN}✅ Token set${NC}"
    echo ""
fi

# Disable git credential prompts
git config --global credential.helper ""

echo "Running publish_docs.sh..."
echo ""

# Run the publish script with token
./publish_docs.sh

echo ""
echo -e "${GREEN}=================================================="
echo "✅ Complete!"
echo "==================================================${NC}"
