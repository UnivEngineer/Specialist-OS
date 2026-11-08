"""Готовые сдвиги всех глифов для автономного FONTBEN; вне измерения."""
from pathlib import Path
import re
import sys
import subprocess

root = Path(__file__).resolve().parents[2]
if '--entries' in sys.argv:
    symbols = dict(re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)', (root / 'bin/FONTBEN.renderer.sym').read_text(), re.M))
    (root / 'bin/FONTBEN.renderer.entries.inc').write_text(''.join(f'CACHE_{name.upper()} = 0{symbols[name]}h\n' for name in ('oneByte', 'twoBytes', 'glyphBody', 'rendererReturn', 'restore')), encoding='utf-8')
    if (root / 'bin/BIOS.sym').exists():
        bios = dict(re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)', (root / 'bin/BIOS.sym').read_text(), re.M))
        (root / 'bin/FONTBEN.bios.entries.inc').write_text(''.join(f'BASE_{alias} = 0{bios[name]}h\n' for alias, name in [('PRINTCHAR_BODY', 'printChar2'), ('DRAWCHAR', 'dos_piece_glyph_1_start')]), encoding='utf-8')
    raise SystemExit
font = (root / 'assets/system-rom/FONT.FNT').read_bytes()
subprocess.run([sys.executable, str(root / 'source-apps/fontbench/analyze-fontkoi.py'), '--build', '--out', str(root / 'docs/bench/fontkoi-build')], check=True, stdout=subprocess.DEVNULL)
assert len(font) == 2048
data = bytearray(0x3000)
for code in range(256):
    for y in range(8):
        v = font[code * 8 + y] & 0x3f
        data[code * 8 + y] = (v << 2) & 0xfc
        data[0x800 + code * 8 + y] = v
        data[0x1000 + code * 16 + y * 2:0x1000 + code * 16 + y * 2 + 2] = bytes((v >> 2, (v << 6) & 0xc0))
        data[0x2000 + code * 16 + y * 2:0x2000 + code * 16 + y * 2 + 2] = bytes((v >> 4, (v << 4) & 0xf0))
(root / 'bin/FONTBEN.shifts.bin').write_bytes(data)
