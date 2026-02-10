# GitHub Setup Guide for Documentation Publishing

Your GitHub credentials (password) won't work directly for API/Git operations. GitHub requires a **Personal Access Token (PAT)** for security.

## Quick Setup (5 minutes)

### Step 1: Create Personal Access Token

1. Go to: https://github.com/settings/tokens
2. Click "Generate new token" → "Generate new token (classic)"
3. Name: `Keraunos_PCIe_Tile_Documentation`
4. Expiration: 90 days (or custom)
5. **Select scopes:**
   - ✅ `repo` (Full control of private repositories)
   - ✅ `workflow` (Update GitHub Action workflows)
6. Click "Generate token"
7. **COPY THE TOKEN** (you won't see it again!)

Example token: `ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`

### Step 2: Run Automated Setup

```bash
cd /localdev/pdroy/keraunos_pcie_workspace

# Export your token
export GITHUB_TOKEN="ghp_your_token_here"

# Run the automated script
./quick_publish.sh
```

## Alternative: Manual Setup

If you prefer to do it manually:

### 1. Create Repository on GitHub Website

1. Go to: https://github.com/new
2. Repository name: `Keraunos_PCIe_TILE`
3. Description: `Keraunos PCIe Tile SystemC/TLM2.0 Documentation with Interactive Diagrams`
4. **Public** (required for free GitHub Pages)
5. Do NOT initialize with README
6. Click "Create repository"

### 2. Push Your Documentation

```bash
cd /localdev/pdroy/keraunos_pcie_workspace

# Add remote (use your token as password)
git remote add origin https://pdroyTT@github.com/pdroyTT/Keraunos_PCIe_TILE.git

# Push (when prompted for password, use your Personal Access Token)
git push -u origin main
```

### 3. Enable GitHub Pages

1. Go to: https://github.com/pdroyTT/Keraunos_PCIe_TILE/settings/pages
2. Source: "Deploy from a branch"
3. Branch: `main`
4. Folder: `/gh-pages`
5. Click "Save"

Wait 1-2 minutes, then view at:
**https://pdroyTT.github.io/Keraunos_PCIe_TILE/**

## Using the Token

Once you have your token, use it as the password:

```bash
Username: pdroyTT  (or pdroy@tenstorrent.com)
Password: ghp_your_token_here  (NOT your GitHub password)
```

## Security Notes

- ✅ Token is more secure than password
- ✅ Can be revoked anytime
- ✅ Specific permissions
- ✅ Expires automatically
- ❌ Never commit tokens to git
- ❌ Never share tokens

## Quick Commands

### Cache credentials (so you don't re-enter):
```bash
git config --global credential.helper 'cache --timeout=7200'
```

### Update documentation:
```bash
./publish_docs.sh
```

### Auto-watch for changes:
```bash
./auto_publish_docs.sh
```

## Troubleshooting

### "Authentication failed"
→ Use Personal Access Token, not password

### "Repository not found"
→ Create repository first (Step 1 above)

### "Permission denied"
→ Ensure token has `repo` scope

### "GitHub Pages not working"
→ Wait 2 minutes, check Settings → Pages

## Files Ready to Publish

✅ All documentation is built and ready:
- `gh-pages/` - Contains HTML documentation
- `docs/_build/html/` - Source HTML files
- All Mermaid diagrams included
- 16 interactive diagrams
- 3 main documents
- Full navigation and search

**Total size:** ~1.5 MB (compressed)
