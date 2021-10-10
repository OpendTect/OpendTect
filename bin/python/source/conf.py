# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
sys.path.insert(0, os.path.abspath('../python'))
print(sys.path)


# -- Project information -----------------------------------------------------

project = 'odpy'
copyright = '2020, dGB Earth Sciences'
author = 'dGB Earth Sciences'

# The full version, including alpha/beta/rc tags
version = '6.6'
release = '6.6'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['autoapi.extension','sphinx.ext.intersphinx']

intersphinx_mapping = {
    'sphinx': ('https://www.sphinx-doc.org/en/master/', None),
    'python': ('https://docs.python.org/3/', None),
}

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# The master toctree document.
master_doc = 'index'

# The name of the Pygments (syntax highlighting) style to use
pygments_style = 'sphinx'


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'
#html_style = 'custom.css'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_logo = '../od_small.png'
html_favicon = None

# -- Autoapi options -------------------------------------------------------

autoapi_type = 'python'
autoapi_dirs = ['../../']

autodoc_default_flags = ['members', 'private-members', 'special-members',
                         #'undoc-members',
                         'show-inheritance']

def autodoc_skip_member(app, what, name, obj, skip, options):
    # Ref: https://stackoverflow.com/a/21449475/
    exclusions = ('__weakref__',  # special-members
                  '__doc__', '__module__', '__dict__', 'tail' # undoc-members
                  )
    exclude = name in exclusions
    # return True if (skip or exclude) else None  # Can interfere with subsequent skip functions.
    return True if exclude else None
 
def setup(app):
    app.connect('autodoc-skip-member', autodoc_skip_member)