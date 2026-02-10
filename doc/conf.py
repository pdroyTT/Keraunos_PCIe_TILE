# Sphinx Configuration file for Keraunos PCIe Tile Documentation
# Generated for Mermaid diagram support
# Compatible with Sphinx 5.x

import os
import sys

# -- Project information -----------------------------------------------------
project = 'Keraunos PCIe Tile'
copyright = '2026, SystemC Modeling Team'
author = 'SystemC Modeling Team'
version = '2.0'
release = '2.0'

# -- General configuration ---------------------------------------------------
extensions = [
    'myst_parser',           # MyST Markdown parser for Sphinx
    'sphinxcontrib.mermaid', # Mermaid diagram support
    'sphinx.ext.todo',       # TODO notes support
    'sphinx.ext.viewcode',   # View code links
    'sphinx.ext.githubpages', # GitHub pages support
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'backup_original']

# -- MyST Parser configuration -----------------------------------------------
# Enable MyST extensions for enhanced Markdown support
# Note: Using extensions compatible with myst-parser 0.15-0.16
myst_enable_extensions = [
    "amsmath",          # LaTeX math support
    "colon_fence",      # Enable ::: fences (required for {mermaid})
    "deflist",          # Definition lists
    "dollarmath",       # Dollar signs for inline math
    # "linkify",        # Auto-link URLs (requires linkify-it-py - disabled)
    "replacements",     # Text replacements
    "smartquotes",      # Smart quotes
    "substitution",     # Variable substitutions
    "tasklist",         # Task lists with checkboxes
]

# MyST heading anchors (compatible with 0.15+)
myst_heading_anchors = 3

# Tell MyST to treat mermaid code blocks as directives
myst_fence_as_directive = ["mermaid"]

# -- Mermaid configuration ---------------------------------------------------
# Mermaid version (use CDN)
mermaid_version = "10.6.1"

# Mermaid initialization
mermaid_init_js = """
mermaid.initialize({
    startOnLoad: true,
    theme: 'default',
    themeVariables: {
        primaryColor: '#e3f2fd',
        primaryTextColor: '#000',
        primaryBorderColor: '#1976d2',
        lineColor: '#1976d2',
        secondaryColor: '#fff4e1',
        tertiaryColor: '#e8f5e9'
    },
    flowchart: {
        useMaxWidth: true,
        htmlLabels: true,
        curve: 'basis',
        padding: 15
    },
    sequence: {
        useMaxWidth: true,
        diagramMarginX: 50,
        diagramMarginY: 10,
        actorMargin: 50,
        boxMargin: 10,
        boxTextMargin: 5,
        noteMargin: 10,
        messageMargin: 35
    },
    gantt: {
        useMaxWidth: true,
        barHeight: 20,
        barGap: 4,
        topPadding: 50,
        leftPadding: 75,
        gridLineStartPadding: 35
    },
    class: {
        useMaxWidth: true
    },
    state: {
        useMaxWidth: true
    }
});
"""

# Mermaid output format - use 'raw' for JavaScript rendering (no mmdc needed!)
mermaid_output_format = 'raw'

# Mermaid verbose mode (useful for debugging)
mermaid_verbose = False

# Don't use mermaid CLI - use JavaScript rendering in browser
mermaid_cmd = None  # Disable CLI rendering

# -- Options for HTML output -------------------------------------------------
# The theme to use for HTML and HTML Help pages.
html_theme = 'sphinx_rtd_theme'

# Theme options (for Read the Docs theme)
html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'includehidden': True,
    'titles_only': False,
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
}

# Add any paths that contain custom static files (such as style sheets)
html_static_path = ['_static']

# Custom CSS (optional - for fine-tuning appearance)
html_css_files = [
    'custom.css',
]

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
# html_logo = '_static/logo.png'

# The name of an image file (within the static path) to use as favicon
# html_favicon = '_static/favicon.ico'

# -- Options for LaTeX output ------------------------------------------------
latex_elements = {
    'papersize': 'letterpaper',
    'pointsize': '10pt',
    'preamble': r'''
\usepackage{charter}
\usepackage[defaultsans]{lato}
\usepackage{inconsolata}
''',
}

# Grouping the document tree into LaTeX files.
latex_documents = [
    ('index', 'KeraunosPcieTile.tex', 
     'Keraunos PCIe Tile Documentation',
     'SystemC Modeling Team', 'manual'),
]

# -- Options for manual page output ------------------------------------------
man_pages = [
    ('index', 'keranouspcietile', 
     'Keraunos PCIe Tile Documentation',
     [author], 1)
]

# -- Options for Texinfo output ----------------------------------------------
texinfo_documents = [
    ('index', 'KeraunosPcieTile', 
     'Keraunos PCIe Tile Documentation',
     author, 'KeraunosPcieTile', 
     'SystemC/TLM2.0 PCIe Tile Implementation',
     'Miscellaneous'),
]

# -- Extension configuration -------------------------------------------------

# TODO extension
todo_include_todos = True
