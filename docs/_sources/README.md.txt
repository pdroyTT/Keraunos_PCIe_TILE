# Documentation Build Instructions

This directory contains the Sphinx configuration for building HTML documentation from the markdown files in the `doc/` folder.

## Prerequisites

Install the required Python packages:

```bash
pip install sphinx myst-parser sphinxcontrib-mermaid sphinx-rtd-theme
```

Or install from the requirements file:

```bash
pip install -r requirements.txt
```

## Building the Documentation

### Using Make (recommended):

```bash
make html
```

### Using Sphinx directly:

```bash
python3 -m sphinx -b html . _build/html
```

## Viewing the Documentation

After building, open `_build/html/index.html` in your web browser.

## Structure

- `conf.py` - Sphinx configuration file
- `index.rst` - Main documentation index
- `*.md` - Source files that include markdown files from `../doc/`
- `_build/html/` - Generated HTML output (created after building)

## Mermaid Diagrams

All diagrams in the markdown files have been converted to Mermaid format and will be rendered as interactive diagrams in the HTML output.

