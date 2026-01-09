#!/usr/bin/env python3
"""
Generate gw/admin_panel.h from contrib/admin-panel.html

Usage: python3 utils/generate_admin_panel_h.py
"""

import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
HTML_FILE = os.path.join(PROJECT_ROOT, 'contrib', 'admin-panel.html')
OUTPUT_FILE = os.path.join(PROJECT_ROOT, 'gw', 'admin_panel.h')

def main():
    with open(HTML_FILE, 'r') as f:
        html = f.read()

    # Escape for C string
    escaped = html.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n"\n"')

    header = '''/*
 * admin_panel.h - Embedded admin panel HTML
 * Auto-generated - do not edit directly
 * Source: contrib/admin-panel.html
 * Generate: python3 utils/generate_admin_panel_h.py
 */

#ifndef ADMIN_PANEL_H
#define ADMIN_PANEL_H

static const char *admin_panel_html =
"''' + escaped + '''";

#endif /* ADMIN_PANEL_H */
'''

    with open(OUTPUT_FILE, 'w') as f:
        f.write(header)

    print(f"Generated {OUTPUT_FILE} from {HTML_FILE}")

if __name__ == '__main__':
    main()
