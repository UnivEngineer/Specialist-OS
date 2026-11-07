"""Прежнее имя проверки; действующая компоновка — BIOS и банковая DOS."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).with_name('check-dos-layout.py')),run_name='__main__')
