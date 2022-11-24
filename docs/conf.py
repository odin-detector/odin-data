# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import odin_data

# -- General configuration ------------------------------------------------

# General information about the project.
project = "odin-data"

# The full version, including alpha/beta/rc tags.
release = odin_data.__version__

# The short X.Y version.
if "+" in release:
    # Not on a tag
    version = "master"
else:
    version = release

# Sphinx Extensions
extensions = [
    # Use this for generating API docs
    "sphinx.ext.autodoc",
    # This can parse google style docstrings
    "sphinx.ext.napoleon",
    # For linking to external sphinx documentation
    "sphinx.ext.intersphinx",
    # Add links to source code in API docs
    "sphinx.ext.viewcode",
    # Adds the inheritance-diagram generation directive
    "sphinx.ext.inheritance_diagram",
    # Add some useful widgets - Cards, Grids, Dropdowns, etc.
    # https://sphinx-design.readthedocs.io
    "sphinx_design",
    # Define full top down hierarchy in sphinx.yaml instead throughout page tree
    # https://sphinx-external-toc.readthedocs.io
    "sphinx_external_toc",
    # Enable Markedly Structured Text
    # https://myst-parser.readthedocs.io
    "myst_parser",
    # Enable breathe for doxygen integration
    "breathe",
]

# Sphinx External TOC Config
external_toc_path = "sphinx.yaml"

# MyST Extensions & Config
myst_enable_extensions = [
    "deflist",
]
## Enable internal link generation for page headings up to this level
myst_heading_anchors = 3

# Breathe Config
breathe_projects = {"odin-data": "cpp/doxygen/_build/xml/"}
breathe_default_project = "odin-data"

# If true, Sphinx will warn about all references where the target cannot
# be found.
nitpicky = True

# A list of (type, target) tuples (by default empty) that should be ignored when
# generating warnings in "nitpicky mode". Note that type should include the
# domain name if present. Example entries would be ('py:func', 'int') or
# ('envvar', 'LD_LIBRARY_PATH').
nitpick_ignore = [("py:func", "int")]

# Both the class’ and the __init__ method’s docstring are concatenated and
# inserted into the main body of the autoclass directive
autoclass_content = "both"

# Order the members by the order they appear in the source code
autodoc_member_order = "bysource"

# Don't inherit docstrings from baseclasses
autodoc_inherit_docstrings = False

# Output graphviz directive produced images in a scalable format
graphviz_output_format = "svg"

# The name of a reST role (builtin or Sphinx extension) to use as the default
# role, that is, for text marked up `like this`
default_role = "any"

# The suffix of source filenames.
source_suffix = ".md"

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# These patterns also affect html_static_path and html_extra_path
exclude_patterns = ["_build"]

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = "sphinx"

# This means you can link things like `str` and `asyncio` to the relevant
# docs in the python documentation.
intersphinx_mapping = dict(python=("https://docs.python.org/3/", None))

# A dictionary of graphviz graph attributes for inheritance diagrams.
inheritance_graph_attrs = dict(rankdir="TB")

# Ignore localhost links for period check that links in docs are valid
linkcheck_ignore = [r"http://localhost:\d+/"]

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "pydata_sphinx_theme"

# Options for theme
github_repo = project
github_user = "odin-detector"
html_theme_options = dict(
    logo=dict(
        text=project,
    ),
    github_url=f"https://github.com/{github_user}/{github_repo}",
    page_sidebar_items=["page-toc"],
    navbar_end=["theme-switcher", "icon-links"],
)

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
# html_static_path = ["_static"]

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = False

# Logo
html_logo = "images/odin.png"
