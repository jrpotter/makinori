# =================================================================================
# Project information

import datetime

project = 'makinori'
copyright = f'{datetime.datetime.now(datetime.timezone.utc).year}, Joshua Potter'
author = 'Joshua Potter'
release = '0.1.0'

# =================================================================================
# General configuration

extensions = []
templates_path = ['_templates']
exclude_patterns = []

# =================================================================================
# Options for HTML output

html_theme = 'alabaster'
html_static_path = ['_static']
