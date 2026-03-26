# Keraunos PCIe Tile Documentation - Sphinx Build Guide

This directory contains the Keraunos PCIe Tile SystemC design documentation with **Mermaid diagrams** for enhanced visualization.

## Quick Start

### 1. Install Dependencies

```bash
# Install Sphinx and required extensions (compatible with Sphinx 5.x+)
pip3 install -r requirements.txt

# Or install minimal versions (fallback)
pip3 install -r requirements-minimal.txt

# Or install manually
pip3 install sphinx sphinx_rtd_theme myst-parser sphinxcontrib-mermaid
```

**Note:** This setup is compatible with Sphinx 5.x and later. See `VERSION_COMPATIBILITY.md` if you encounter version issues.

### 2. Build HTML Documentation

```bash
# From the doc/ directory
make html

# Or use sphinx-build directly
sphinx-build -b html . _build/html
```

### 3. View Documentation

```bash
# Option 1: Open directly
open _build/html/index.html

# Option 2: Serve with HTTP server
make serve
# Then open: http://localhost:8000
```

## What's Included

### Mermaid Diagrams

The documentation now includes **interactive Mermaid diagrams** showing:

- ✅ **Architecture Diagrams** - Component hierarchy and relationships
- ✅ **Flow Diagrams** - Data flow through TLBs, switches, and MSI relay
- ✅ **Sequence Diagrams** - Transaction processing sequences
- ✅ **State Machines** - MSI thrower logic, reset sequences
- ✅ **Class Diagrams** - Component relationships and ownership

### Documentation Structure

```
doc/
├── conf.py                          # Sphinx configuration (Mermaid enabled)
├── index.rst                        # Table of contents
├── Makefile                         # Build commands
├── _static/
│   └── custom.css                   # Custom styling for diagrams
├── Keraunos_PCIE_Tile_SystemC_Design_Document.md  # Main documentation
└── _build/
    └── html/                        # Generated HTML (after build)
```

## Build Commands

### Standard Build

```bash
make html          # Build HTML documentation
make clean         # Clean build artifacts
make serve         # Build and serve on http://localhost:8000
```

### Advanced Options

```bash
# Live reload (auto-rebuild on file changes)
make livehtml

# Build PDF (requires LaTeX)
make latexpdf

# Build multiple formats
make html latexpdf epub
```

## Mermaid Configuration

The Mermaid diagrams are configured in `conf.py`:

```python
mermaid_version = "10.6.1"  # Latest stable version
mermaid_output_format = 'svg'  # High-quality SVG output
```

### Customizing Diagrams

Edit the diagram themes in `conf.py`:

```python
mermaid_init_js = """
mermaid.initialize({
    theme: 'default',  # or 'dark', 'forest', 'neutral'
    themeVariables: {
        primaryColor: '#e3f2fd',
        primaryBorderColor: '#1976d2',
        // ... customize colors
    }
});
"""
```

## Features

### Enhanced Visualization

- **Flowcharts**: Show routing logic and decision paths
- **Sequence Diagrams**: Illustrate transaction flows
- **State Machines**: Display MSI thrower and reset sequences
- **Class Diagrams**: Show component relationships

### Responsive Design

- Diagrams scale to fit screen width
- Mobile-friendly layout
- Print-optimized styling

### Search and Navigation

- Full-text search across all documentation
- Collapsible sidebar navigation
- Breadcrumb navigation
- Anchor links to all sections

## Theme Customization

The documentation uses the **Read the Docs** theme with custom styling:

```python
# In conf.py
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
}
```

### Available Themes

To change the theme, modify `conf.py`:

```python
html_theme = 'alabaster'     # Clean, minimal
html_theme = 'pydata_sphinx_theme'  # Modern, for data projects
html_theme = 'furo'          # Modern, clean
```

## Troubleshooting

### Mermaid Diagrams Not Rendering

1. Check that `sphinxcontrib-mermaid` is installed:
   ```bash
   pip show sphinxcontrib-mermaid
   ```

2. Verify Mermaid syntax in the markdown:
   ```markdown
   ```{mermaid}
   graph TD
       A --> B
   ```
   ```

3. Check build output for errors:
   ```bash
   make html 2>&1 | grep -i error
   ```

### MyST Parser Errors

If you see "Unknown directive type" errors:

1. Ensure `myst-parser` is installed
2. Check `conf.py` has `myst_parser` in extensions
3. Verify markdown syntax (proper fences, spacing)

### CSS Not Loading

If custom styles aren't applied:

1. Check `_static/custom.css` exists
2. Verify it's listed in `conf.py`:
   ```python
   html_css_files = ['custom.css']
   ```
3. Clear build cache: `make clean && make html`

## Development Workflow

### Live Editing

For real-time preview during editing:

```bash
# Terminal 1: Start live reload
make livehtml

# Terminal 2: Edit markdown files
vim Keraunos_PCIE_Tile_SystemC_Design_Document.md
```

Browser automatically refreshes when files change.

### Adding New Diagrams

1. **Choose Diagram Type**:
   - `graph TD` - Top-down flowchart
   - `sequenceDiagram` - Sequence diagram
   - `stateDiagram-v2` - State machine
   - `classDiagram` - Class relationships

2. **Add to Markdown**:
   ```markdown
   ```{mermaid}
   graph TD
       A[Start] --> B[Process]
       B --> C[End]
   ```
   ```

3. **Rebuild and Verify**:
   ```bash
   make html
   open _build/html/index.html
   ```

## Production Deployment

### GitHub Pages

```bash
# Build for GitHub Pages
make html
touch _build/html/.nojekyll
cp -r _build/html/* /path/to/gh-pages-branch/
```

### Read the Docs

1. Connect your repository to Read the Docs
2. Configure build:
   ```yaml
   # .readthedocs.yaml
   version: 2
   sphinx:
     configuration: doc/conf.py
   python:
     install:
       - requirements: doc/requirements.txt
   ```

3. Push to trigger build

## Support

For issues with:
- **Sphinx**: https://www.sphinx-doc.org/
- **Mermaid**: https://mermaid.js.org/
- **MyST Parser**: https://myst-parser.readthedocs.io/

## Summary

Your documentation now features:

✅ **Interactive Mermaid diagrams** for visual clarity  
✅ **Professional HTML output** with Read the Docs theme  
✅ **Full-text search** across all content  
✅ **Responsive design** for all devices  
✅ **Easy customization** via `conf.py` and CSS  
✅ **Multiple output formats** (HTML, PDF, EPUB)  

Build beautiful, professional documentation with:
```bash
make html && open _build/html/index.html
```

Enjoy your enhanced documentation! 🎉
