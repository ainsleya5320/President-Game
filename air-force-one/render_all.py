"""Render all four cameras from the loaded master; cutaway visibility changes last."""
import sys,runpy
from pathlib import Path
for mode in ['hallway','office','conference','cutaway']:
    sys.argv=['render_and_export.py',mode]
    runpy.run_path(str(Path(__file__).with_name('render_and_export.py')),run_name='__main__')
