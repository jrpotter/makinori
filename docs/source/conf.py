# =================================================================================
# Project information

import datetime

project = 'makinori'
author = 'Joshua Potter, Brittany Colonna'
copyright = f'{datetime.datetime.now(datetime.timezone.utc).year} {author}'
release = '0.1.0'

# =================================================================================
# General configuration

exclude_patterns = []
extensions = []
maximum_signature_line_length = 80
templates_path = ['_templates']

def setup(app):
    app.add_css_file('makinori.css')

# =================================================================================
# Options for HTML output

html_theme = 'alabaster'
html_static_path = ['_static']
