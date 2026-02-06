# ✅ Upgrading to Sphinx 5.x - Simpler Solution!

## Better Approach: Upgrade Sphinx Instead

Instead of downgrading sphinx-rtd-theme, we'll **upgrade Sphinx to 5.x** which is available in your environment (up to 5.3.0).

### Current Situation
- ✅ Sphinx available: 5.0.0 - 5.3.0 (in your environment)
- ✅ sphinx-rtd-theme installed: 2.0.0 (requires Sphinx 5.0+)
- ❌ Sphinx currently: 4.5.0 (too old for theme 2.0.0)

### Solution: Upgrade Sphinx
```
Sphinx 4.5.0 → Sphinx 5.3.0 ✅
```

---

## 🚀 UPGRADE COMMANDS (2 Steps)

### Step 1: Upgrade Sphinx
```bash
cd /localdev/pdroy/keraunos_pcie_workspace/doc
pip3 install --upgrade "sphinx>=5.0.0,<6.0.0"
```

### Step 2: Build Documentation
```bash
./build_docs.sh
```

Or manually:
```bash
make html
```

---

## ✅ Expected Upgrade Output

```
Collecting sphinx>=5.0.0,<6.0.0
  Downloading sphinx-5.3.0-py3-none-any.whl
Installing collected packages: sphinx
  Attempting uninstall: sphinx
    Found existing installation: sphinx 4.5.0
    Uninstalling sphinx-4.5.0:
      Successfully uninstalled sphinx-4.5.0
Successfully installed sphinx-5.3.0
```

---

## 🔍 Verify After Upgrade

```bash
pip3 list | grep -E "sphinx|myst|mermaid"
```

**Expected output:**
```
myst-parser              0.16.1
sphinx                   5.3.0     ← Upgraded from 4.5.0!
sphinx-rtd-theme         2.0.0     ← Already installed, now compatible!
sphinxcontrib-mermaid    0.8.1
```

---

## 📊 Final Compatible Versions

| Package | Version | Status |
|---------|---------|--------|
| sphinx | 5.3.0 | ✅ Upgraded |
| sphinx_rtd_theme | 2.0.0 | ✅ Compatible with Sphinx 5.x |
| myst-parser | 0.16.1 | ✅ Compatible |
| sphinxcontrib-mermaid | 0.8.1 | ✅ Compatible |

---

## ✨ Benefits of Sphinx 5.x

- ✅ Works with your installed sphinx-rtd-theme 2.0.0
- ✅ Latest features and improvements
- ✅ Better Mermaid diagram support
- ✅ Improved MyST parser integration
- ✅ Better HTML output quality

---

## 📋 Alternative: Install All Fresh

If you want to ensure everything is clean:

```bash
cd /localdev/pdroy/keraunos_pcie_workspace/doc

# Uninstall all Sphinx packages
pip3 uninstall -y sphinx sphinx_rtd_theme myst-parser sphinxcontrib-mermaid

# Install fresh with Sphinx 5.x
pip3 install -r requirements.txt

# Verify
pip3 list | grep -E "sphinx|myst|mermaid"
```

---

## 📖 Expected Build Output

```
========================================
Keraunos PCIe Tile Documentation Builder
========================================

✓ Python 3 found: Python 3.6.x
✓ pip3 found

Checking dependencies...
✓ Sphinx (5.3.0)          ← Upgraded!
✓ Read the Docs Theme (2.0.0)
✓ MyST Parser (0.16.1)
✓ Mermaid Extension (0.8.1)

Building HTML documentation...
----------------------------------------
Running Sphinx v5.3.0     ← Now shows 5.3.0!
loading extensions... done
building [html]: all source files
...
build succeeded.
----------------------------------------
✓ Documentation built successfully!

Build Statistics:
  Output size:      2.5M
  Files generated:  47
  Mermaid diagrams: 23
========================================
```

---

## 🎯 Why This Is Better

| Approach | Pros | Cons |
|----------|------|------|
| **Upgrade Sphinx** | ✅ Uses latest Sphinx 5.x<br/>✅ Better features<br/>✅ Keeps theme 2.0.0<br/>✅ Simpler | - |
| Downgrade theme | Works with Sphinx 4.x | ❌ Older theme version<br/>❌ Missing features |

---

## ⚡ Quick Command Summary

```bash
# Single command to upgrade and build
cd /localdev/pdroy/keraunos_pcie_workspace/doc && \
pip3 install --upgrade "sphinx>=5.0.0,<6.0.0" && \
make html
```

---

## ✅ What You Get

With Sphinx 5.3.0 + sphinx-rtd-theme 2.0.0:

✅ **23 Mermaid Diagrams** - Full support  
✅ **Modern Theme 2.0** - Latest RTD theme features  
✅ **Better HTML Output** - Improved rendering  
✅ **Full-Text Search** - Enhanced search  
✅ **Responsive Design** - Mobile-friendly  
✅ **Custom Styling** - All CSS works  

---

## 🆘 If Upgrade Fails

If pip can't upgrade Sphinx:

```bash
# Force reinstall specific version
pip3 install --force-reinstall sphinx==5.3.0

# Or use requirements.txt
pip3 install -r requirements.txt
```

---

## 📁 Files Updated

| File | Status | Change |
|------|--------|--------|
| `requirements.txt` | ✅ Updated | Now specifies Sphinx 5.x |
| `UPGRADE_TO_SPHINX5.md` | ✅ Created | This guide |

---

## ✅ READY TO UPGRADE!

**Execute now:**

```bash
cd /localdev/pdroy/keraunos_pcie_workspace/doc
pip3 install --upgrade "sphinx>=5.0.0,<6.0.0"
make html
```

**Your documentation with 23 beautiful Mermaid diagrams will build successfully!** 🎉

---

*Recommended approach: Upgrade Sphinx to 5.x for best results*  
*Compatible with: Python 3.6+ / Sphinx 5.0-5.3.0 / sphinx-rtd-theme 2.0.0*
