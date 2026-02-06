# Configuration file for the Sphinx documentation builder.
#
# Keraunos PCIe Tile Documentation - Clean Version
# Focuses on: High-Level Design, SystemC Design, and Test Plan

import os
import sys
sys.path.insert(0, os.path.abspath('.'))

# -- Project information -----------------------------------------------------

project = 'Keraunos PCIe Tile'
copyright = '2026, Keraunos PCIe Tile Team'
author = 'Keraunos PCIe Tile Team'
release = '1.0'
version = '1.0'

# -- General configuration ---------------------------------------------------

extensions = [
    'myst_parser',             # MyST Markdown parser for .md files
    'sphinxcontrib.mermaid',   # Mermaid diagram support
]

# MyST Parser configuration - enable extended syntax
myst_enable_extensions = [
    "colon_fence",      # ::: fences for directives
    "deflist",          # Definition lists
    "fieldlist",        # Field lists
    "html_admonition",  # HTML-style admonitions
    "html_image",       # HTML-style images
    # "linkify",        # Auto-detect URLs (requires linkify-it-py package)
    "replacements",     # Character replacements
    "smartquotes",      # Smart quotes
    "substitution",     # Substitutions
    "tasklist",         # Task lists
]

# Mermaid configuration
mermaid_version = "10.6.1"
mermaid_init_js = """
mermaid.initialize({
    startOnLoad: true,
    theme: 'default',
    securityLevel: 'loose',
    flowchart: {
        useMaxWidth: true,
        htmlLabels: true,
        curve: 'basis',
        nodeSpacing: 70,
        rankSpacing: 70,
        padding: 20
    },
    fontSize: 16,
    themeVariables: {
        fontSize: '16px',
        primaryColor: '#e1f5ff',
        primaryTextColor: '#000',
        primaryBorderColor: '#3498db',
        lineColor: '#2c3e50',
        secondaryColor: '#fff4e1',
        tertiaryColor: '#ffe1f5'
    }
});
"""
mermaid_output_format = 'raw'

# Templates path
templates_path = ['_templates']

# Patterns to ignore when looking for source files
exclude_patterns = [
    '_build',
    'Thumbs.db',
    '.DS_Store',
    'BUILD_*.md',
    'FINAL_*.md',
    'SUCCESS_*.md',
    'UPGRADE_*.md',
    'SPHINX_*.md',
    'VERSION_*.md',
    'MERMAID_*.md',
    'PDF_*.md',
    'README_*.md',
    'Why_*.md',
    'SCML2_*.md',
    'VZ_*.md',
    'PAVP_*.md',
    'VDK_*.md',
    'TLB_*.md',
    'SCML_*.md',
    'Implementation_*.md',
    'DBI_*.md',
    'SII_*.md',
    'VIEW_*.md',
    'requirements.txt',
]

# Source file extensions
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

# The master toctree document
master_doc = 'index'

# -- Options for HTML output -------------------------------------------------

html_theme = 'alabaster'

html_theme_options = {
    'description': 'SystemC/TLM2.0 PCIe Tile Design and Verification',
    'github_button': False,
    'fixed_sidebar': True,
    'sidebar_width': '250px',
    'page_width': '1200px',
}

html_static_path = ['_static']
html_css_files = ['custom.css']

# HTML title
html_title = f"{project} v{release}"
html_short_title = project

# Sidebar settings
html_sidebars = {
    '**': [
        'about.html',
        'navigation.html',
        'relations.html',
        'searchbox.html',
    ]
}

# -- Options for LaTeX/PDF output --------------------------------------------

latex_elements = {
    'papersize': 'a4paper',
    'pointsize': '10pt',
}

latex_documents = [
    (master_doc, 'KeraunosPCIeTile.tex', 'Keraunos PCIe Tile Documentation',
     author, 'manual'),
]
