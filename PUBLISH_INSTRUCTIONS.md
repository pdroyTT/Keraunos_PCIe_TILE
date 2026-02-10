# Publishing Documentation to GitHub - Complete Instructions

**Status:** ✅ Documentation built and ready  
**Date:** February 6, 2026  

---

## Important: GitHub Authentication

GitHub **no longer accepts passwords** for Git operations. You need a **Personal Access Token (PAT)**.

### Your provided credentials won't work because:
- ❌ Password: `KairuHanika@2025` - GitHub blocks password authentication
- ✅ Required: Personal Access Token (PAT)

---

## Quick Setup (2 Steps)

### Step 1: Get Your Personal Access Token (2 minutes)

1. **Go to:** https://github.com/settings/tokens/new
2. **Login with:** 
   - Username: `pdroy@tenstorrent.com`
   - Password: `KairuHanika@2025`
3. **Token settings:**
   - Note: `Keraunos_Documentation`
   - Expiration: `90 days`
   - Select scopes:
     - ✅ **repo** (all sub-options)
     - ✅ **workflow**
4. **Click:** "Generate token"
5. **COPY THE TOKEN** immediately (starts with `ghp_`)

**Example token:** `ghp_1234567890abcdefghijklmnopqrstuvwxyz`

### Step 2: Publish Documentation

```bash
cd /localdev/pdroy/keraunos_pcie_workspace

# Set your token (replace with your actual token)
export GITHUB_TOKEN="ghp_your_token_from_step1"

# Run the automated script
./publish_docs.sh
```

**That's it!** The script will:
1. Create the repository on GitHub
2. Push all documentation
3. Configure GitHub Pages
4. Give you the URL

---

## Alternative: Manual Method

If you prefer to do it step-by-step:

### 1. Create Repository (Website)

1. Go to: https://github.com/new
2. Repository name: `Keraunos_PCIe_TILE`
3. Description: `SystemC/TLM2.0 Documentation with Interactive Diagrams`
4. **Public** repository
5. **Do NOT** initialize with README
6. Click "Create repository"

### 2. Push Your Code

```bash
cd /localdev/pdroy/keraunos_pcie_workspace

# Add remote
git remote add origin https://github.com/pdroyTT/Keraunos_PCIe_TILE.git

# Push (use your PAT as password when prompted)
git push -u origin main
```

### 3. Enable GitHub Pages

1. Go to: https://github.com/pdroyTT/Keraunos_PCIe_TILE/settings/pages
2. **Source:** Deploy from a branch
3. **Branch:** main
4. **Folder:** /gh-pages
5. Click "Save"

Wait 1-2 minutes, then view at:
**https://pdroyTT.github.io/Keraunos_PCIe_TILE/**

---

## What's Ready to Publish

✅ **All Documentation Built:**
- `gh-pages/` directory contains complete HTML
- `docs/_build/html/` has all source files
- 16 interactive Mermaid diagrams included
- 3 main documents (HLD, SystemC, Test Plan)
- Professional styling applied
- Enhanced diagrams (800-1200px, 16-18px fonts)

✅ **File sizes:**
- Total: ~1.5 MB
- Main page: 52 KB
- All diagrams embedded
- Ready for GitHub Pages

✅ **Scripts Ready:**
- `publish_docs.sh` - One-time publish
- `auto_publish_docs.sh` - Auto-watch and publish
- All tested and working

---

## Troubleshooting

### "Authentication failed"
→ Use Personal Access Token, not password  
→ Get token from: https://github.com/settings/tokens

### "Repository already exists"
→ Good! Just push: `git push origin main`

### "Permission denied"
→ Check token has `repo` scope selected

### "Remote already exists"
→ Update it: `git remote set-url origin https://github.com/pdroyTT/Keraunos_PCIe_TILE.git`

### GitHub Pages not working
→ Wait 2 minutes after enabling
→ Check: https://github.com/pdroyTT/Keraunos_PCIe_TILE/settings/pages
→ Ensure branch is "main" and folder is "/gh-pages"

---

## Commands Cheat Sheet

```bash
# Get your current status
cd /localdev/pdroy/keraunos_pcie_workspace
git status

# View documentation locally
firefox gh-pages/index.html

# Rebuild documentation
cd docs
sphinx-build -b html . _build/html
cd ..

# Publish (with token)
export GITHUB_TOKEN="ghp_your_token"
./publish_docs.sh

# Auto-publish on changes
./auto_publish_docs.sh
```

---

## Security Notes

### ✅ DO:
- Use Personal Access Token for Git/API operations
- Store token securely (not in files)
- Set token expiration (90 days recommended)
- Revoke tokens when done

### ❌ DON'T:
- Use GitHub password for Git operations (blocked)
- Commit tokens to repository
- Share tokens publicly
- Use tokens without expiration

---

## What Happens After Publishing

1. **Immediate:**
   - Code pushed to GitHub
   - Repository visible at: https://github.com/pdroyTT/Keraunos_PCIe_TILE

2. **After 1-2 minutes:**
   - GitHub Pages builds your site
   - Documentation available at: https://pdroyTT.github.io/Keraunos_PCIe_TILE/

3. **Features Available:**
   - 16 interactive Mermaid diagrams
   - Full-text search
   - Mobile responsive
   - Professional styling
   - Fast loading

---

## Next Steps After Publishing

1. **Share the link:**
   ```
   https://pdroyTT.github.io/Keraunos_PCIe_TILE/
   ```

2. **Update documentation:**
   ```bash
   # Edit files in docs/
   vim docs/hld.md
   
   # Publish updates
   ./publish_docs.sh
   ```

3. **Auto-publish mode:**
   ```bash
   # Watches for changes and auto-publishes
   ./auto_publish_docs.sh
   ```

---

## Summary

**✅ Ready:** All documentation built and scripts configured  
**📝 Action:** Get Personal Access Token from GitHub  
**⏱️  Time:** 5 minutes total  
**🎯 Result:** Professional documentation website live  

**URL after publishing:**
```
https://pdroyTT.github.io/Keraunos_PCIe_TILE/
```

---

## Support

If you encounter issues:

1. Check GITHUB_SETUP_GUIDE.md for detailed instructions
2. Verify token has correct permissions
3. Ensure repository is public (required for free GitHub Pages)
4. Wait 2 minutes after enabling GitHub Pages

**Documentation is 100% ready to publish!**
