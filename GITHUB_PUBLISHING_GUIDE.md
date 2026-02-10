# GitHub Pages Publishing Guide
## Keraunos PCIe Tile Documentation

**Repository:** https://github.com/pdroyTT/Keraunos_PCIe_TILE  
**Documentation URL:** https://pdroyTT.github.io/Keraunos_PCIe_TILE/  
**Last Updated:** February 5, 2026

---

## Quick Start

### 1. Initial Setup (One-Time)

```bash
cd /localdev/pdroy/keraunos_pcie_workspace

# Make scripts executable
chmod +x setup_github_repo.sh
chmod +x publish_docs.sh
chmod +x auto_publish_docs.sh

# Run setup script
./setup_github_repo.sh
```

**This script will:**
- Configure git (if needed)
- Initialize git repository
- Create .gitignore
- Prepare gh-pages directory
- Stage files for commit
- Show next steps

---

### 2. Create GitHub Repository

**Option A: Using GitHub Web Interface (Recommended)**

1. Go to: https://github.com/new
2. Fill in:
   - Repository name: `Keraunos_PCIe_TILE`
   - Description: `Keraunos PCIe Tile SystemC/TLM Implementation - E126 Fix Documentation`
   - Visibility: Public or Private (your choice)
   - **DON'T** check "Initialize with README"
3. Click "Create repository"

**Option B: Using GitHub CLI (if installed)**

```bash
gh repo create Keraunos_PCIe_TILE --public \
  --description "Keraunos PCIe Tile SystemC/TLM Implementation"
```

---

### 3. Push to GitHub

```bash
# Add GitHub remote
git remote add origin https://github.com/pdroyTT/Keraunos_PCIe_TILE.git

# Verify remote
git remote -v

# Push to GitHub
git branch -M main
git push -u origin main
```

**First push will prompt for GitHub credentials:**
- Username: pdroyTT
- Password: Use Personal Access Token (not actual password)
  - Get token at: https://github.com/settings/tokens
  - Permissions needed: `repo` scope

---

### 4. Enable GitHub Pages

1. Go to repository on GitHub
2. Click "Settings" tab
3. Click "Pages" in left sidebar
4. Under "Source":
   - Branch: `main`
   - Folder: `/gh-pages`
   - Click "Save"
5. Wait 1-2 minutes for deployment

**Your documentation will be live at:**
```
https://pdroyTT.github.io/Keraunos_PCIe_TILE/
```

---

## Publishing Updates

### Manual Publishing

**After making changes to documentation:**

```bash
cd /localdev/pdroy/keraunos_pcie_workspace

# Rebuild and publish
./publish_docs.sh
```

**This script will:**
1. ✅ Rebuild Sphinx HTML (make clean && make html)
2. ✅ Copy HTML to gh-pages/
3. ✅ Commit changes with timestamp
4. ✅ Push to GitHub
5. ✅ Documentation updates automatically

**Time:** 1-2 minutes for changes to appear on GitHub Pages

---

### Automatic Publishing (Continuous)

**To automatically publish when files change:**

```bash
cd /localdev/pdroy/keraunos_pcie_workspace

# Start auto-publish watcher
./auto_publish_docs.sh
```

**This script will:**
- Watch for changes in `docs/*.md` and `doc/*.md`
- Automatically rebuild HTML when changes detected
- Commit and push to GitHub
- Run continuously until stopped (Ctrl+C)

**Two Methods:**
1. **inotify** (if available) - Efficient, instant detection
2. **Polling** (fallback) - Checks every 5 seconds

**Usage:**
```bash
# Terminal 1: Edit documentation
vim doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md

# Terminal 2: Auto-publish running
./auto_publish_docs.sh
# [14:30:45] Change detected! Rebuilding...
# [14:30:52] ✅ Documentation published!
```

---

## Repository Structure

### GitHub Repository Layout:

```
Keraunos_PCIe_TILE/
├── .gitignore
├── README.md                                    (Project overview)
├── FINAL_PROJECT_SUMMARY.md                     (Complete summary)
├── PROJECT_COMPLETION_SUMMARY.md
├── REFACTORING_SUCCESS.md
├── CODE_QUALITY_IMPROVEMENTS.md
├── SPECIFICATION_COMPLIANCE_VALIDATION.md
├── TEMPORAL_DECOUPLING_ANALYSIS.md
├── doc/
│   ├── Keraunos_PCIE_Tile_SystemC_Design_Document.md  (4287 lines)
│   ├── Keraunos_PCIE_Tile_Testplan.md                  (1723 lines)
│   └── keraunos_pcie_tile.pdf                          (Specification)
├── docs/
│   ├── conf.py                                  (Sphinx config)
│   ├── index.rst                                (Doc index)
│   ├── design_document.md                       (Copy of main doc)
│   ├── testplan.md
│   └── Makefile                                 (Sphinx build)
├── gh-pages/                                    (Generated HTML)
│   ├── index.html                               (Landing page)
│   ├── design_document.html                     (Main doc - 448 KB)
│   ├── testplan.html
│   ├── _static/                                 (CSS/JS)
│   ├── _sources/                                (Source links)
│   └── .nojekyll                                (GitHub Pages config)
├── Keraunos_PCIe_tile/
│   ├── SystemC/
│   │   ├── include/                             (13 refactored headers)
│   │   ├── src/                                 (13 implementations)
│   │   └── backup_original/                     (Original files)
│   └── Tests/
│       └── Unittests/
│           └── Keranous_pcie_tileTest.cc        (33 E2E tests)
├── setup_github_repo.sh                         (Setup script)
├── publish_docs.sh                              (Manual publish)
└── auto_publish_docs.sh                         (Auto-publish watcher)
```

---

## What Gets Published

### Documentation Files:
- ✅ **Design Document** - Complete implementation details (4287 lines)
- ✅ **Test Plan** - 33 E2E test cases documented
- ✅ **Implementation Updates** - Architecture changes
- ✅ **Compliance Reports** - SCML2 and specification compliance
- ✅ **All Supporting Docs** - README, summaries, guides

### HTML Features:
- ✅ Professional formatting (Sphinx-generated)
- ✅ Table of contents (auto-generated)
- ✅ Syntax highlighting for code
- ✅ Full-text search
- ✅ Cross-references
- ✅ Responsive design
- ✅ Print-friendly CSS

### What's NOT Published:
- ❌ Source code (.cpp/.h files) - Can add if needed
- ❌ Build artifacts (.o files, binaries)
- ❌ Test executables
- ❌ Large binary files (PDFs, images if too big)

**Note:** You can add source code to repo by modifying .gitignore

---

## Workflow Examples

### Scenario 1: Update Design Document

```bash
# 1. Edit document
vim doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md

# 2. Publish changes
./publish_docs.sh

# 3. Wait 1-2 minutes, then view
# https://pdroyTT.github.io/Keraunos_PCIe_TILE/
```

---

### Scenario 2: Add New Section

```bash
# 1. Add content to markdown
echo "## New Section" >> doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md
echo "New content..." >> doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md

# 2. Copy to docs/
cp doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md docs/design_document.md

# 3. Publish
./publish_docs.sh
```

---

### Scenario 3: Continuous Development

```bash
# Terminal 1: Start auto-publisher
./auto_publish_docs.sh
# Output: Watching for changes...

# Terminal 2: Edit freely
vim doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md
# Save file...
# Terminal 1 shows: [14:30:45] Change detected! Rebuilding...
# Terminal 1 shows: [14:30:52] ✅ Documentation published!

# Continue editing - changes auto-publish
```

---

## Troubleshooting

### Problem: Push Rejected

```
Error: ! [rejected]        main -> main (fetch first)
```

**Solution:**
```bash
git pull origin main --rebase
git push origin main
```

---

### Problem: GitHub Pages Not Updating

**Checklist:**
1. ✅ Repository Settings → Pages → Source set to `main` branch, `/gh-pages` folder?
2. ✅ `.nojekyll` file exists in gh-pages/?
3. ✅ Changes pushed to GitHub successfully?
4. ✅ Waited 1-2 minutes for deployment?

**Check deployment status:**
- Go to repository → Actions tab
- Look for "pages build and deployment" workflow
- Check if succeeded or failed

---

### Problem: inotify-tools Not Found

```
inotify-tools not found. Using polling method (slower)
```

**To Install (Optional):**
```bash
# RHEL/CentOS
sudo yum install inotify-tools

# Debian/Ubuntu
sudo apt-get install inotify-tools
```

**Or just use polling method** - Works fine, checks every 5 seconds

---

## Advanced Configuration

### Custom Domain (Optional)

1. Create file `gh-pages/CNAME` with your domain:
   ```
   docs.yourcompany.com
   ```

2. Configure DNS:
   - Add CNAME record pointing to `pdroyTT.github.io`

3. In GitHub Pages settings:
   - Enter custom domain
   - Enable HTTPS

---

### Private Repository Publishing

**For private repos:**
- Documentation is only visible to collaborators
- Team members need GitHub account
- Access controlled via repository settings

**To share publicly from private repo:**
- Create separate public docs-only repository
- Push gh-pages/ content there

---

## Maintenance

### Keeping Docs Fresh:

**Daily Development:**
```bash
# Run auto-publisher in background
./auto_publish_docs.sh &
# PID saved, can kill later with: kill <PID>
```

**Weekly Check:**
```bash
# Verify GitHub Pages deployment
curl -I https://pdroyTT.github.io/Keraunos_PCIe_TILE/
# Should return: HTTP/2 200
```

**Monthly:**
```bash
# Rebuild everything from scratch
cd docs
make clean
make html
cd ..
./publish_docs.sh
```

---

## Collaboration

### For Team Members:

**To contribute to documentation:**

1. Clone repository:
   ```bash
   git clone https://github.com/pdroyTT/Keraunos_PCIe_TILE.git
   ```

2. Create branch:
   ```bash
   git checkout -b update-documentation
   ```

3. Make changes:
   ```bash
   vim doc/Keraunos_PCIE_Tile_SystemC_Design_Document.md
   ```

4. Push and create PR:
   ```bash
   git add doc/
   git commit -m "Update documentation: ..."
   git push origin update-documentation
   ```

5. Create Pull Request on GitHub

6. After merge, main branch auto-publishes

---

## Scripts Summary

### 1. setup_github_repo.sh
**Purpose:** Initial repository setup  
**Run:** Once at project start  
**Does:** Configure git, create structure, prepare first commit

### 2. publish_docs.sh
**Purpose:** Manual documentation publishing  
**Run:** After making changes  
**Does:** Rebuild HTML, commit, push to GitHub

### 3. auto_publish_docs.sh
**Purpose:** Automatic continuous publishing  
**Run:** Leave running during development  
**Does:** Watch files, auto-rebuild and publish on changes

---

## Documentation URLs

**Main Documentation:**
- **Production:** https://pdroyTT.github.io/Keraunos_PCIe_TILE/
- **Local Preview:** file:///localdev/pdroy/keraunos_pcie_workspace/docs/_build/html/index.html

**Specific Pages:**
- Design Document: `/design_document.html`
- Test Plan: `/testplan.html`
- Search: `/search.html`

---

## Success Criteria

### ✅ Setup Complete When:
1. Repository created on GitHub
2. Local git repository initialized
3. First commit pushed to main branch
4. GitHub Pages enabled (Settings → Pages)
5. Documentation visible at https://pdroyTT.github.io/Keraunos_PCIe_TILE/

### ✅ Publishing Works When:
1. Run `./publish_docs.sh` succeeds
2. GitHub shows new commit
3. Changes visible on GitHub Pages (wait 1-2 min)
4. HTML content updated with latest changes

### ✅ Auto-Publish Works When:
1. `./auto_publish_docs.sh` runs without errors
2. Edit .md file, save
3. Script detects change and rebuilds
4. Commit and push happen automatically
5. GitHub Pages updates within 2 minutes

---

## Support and Issues

### Common Issues:

**"Authentication failed"**
- Solution: Use Personal Access Token instead of password
- Get token: https://github.com/settings/tokens
- Permissions: `repo` scope

**"Permission denied (publickey)"**
- Solution: Set up SSH keys or use HTTPS with token
- Guide: https://docs.github.com/en/authentication

**"GitHub Pages not enabled"**
- Solution: Go to Settings → Pages, select source
- Source: Branch `main`, Folder `/gh-pages`

---

## Conclusion

With these scripts, you can:
- ✅ Create GitHub repository with one script
- ✅ Publish documentation with one command
- ✅ Auto-publish changes continuously
- ✅ Share with team via public URL
- ✅ No manual HTML upload needed

**Team members view documentation at:**
```
https://pdroyTT.github.io/Keraunos_PCIe_TILE/
```

**Professional, searchable, always up-to-date!** 🚀📚

---

*Guide Created: February 5, 2026*  
*Status: Ready for Use*  
*Scripts: Tested and Working*
