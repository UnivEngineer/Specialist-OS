"""Адреса текущей сборки из mxos.inc через DOS.sym, без копий карты памяти."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
MEMORY_MAP = {k: int(v, 16) for k, v in re.findall(
    r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
    (ROOT/'bin/DOS.sym').read_text(encoding='utf-8-sig'), re.M)}
