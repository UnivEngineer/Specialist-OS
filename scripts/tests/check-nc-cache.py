"""Совместимый вход CPU-стенда общего кэша каталогов NC.

Актуальные сценарии находятся в check-nc-catalog.py; --prepare-only не
запускает CPU-стенд. Старый --report-directory принимается как --output.
"""
from pathlib import Path
import runpy
import sys

if __name__ == '__main__':
    target = Path(__file__).with_name('check-nc-catalog.py')
    sys.argv = [str(target)] + [argument.replace('--report-directory', '--output', 1)
                               if argument.startswith('--report-directory') else argument
                               for argument in sys.argv[1:]]
    runpy.run_path(str(target), run_name='__main__')
