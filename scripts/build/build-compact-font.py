"""Создание внешнего системного шрифта; существующий пользовательский сохраняется."""
from pathlib import Path
import argparse
ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--force', action='store_true', help='Восстановить шрифт из полного FONT.FNT')
a = p.parse_args()
source = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
assert len(source) == 2048
ranges = source[0x20*8:0x80*8] + source[0xc0*8:0x100*8]
yo = source[0xa3*8:0xa4*8] + source[0xb3*8:0xb4*8]
target = ROOT / 'assets/system-rom/FONTKOI.FNT'
import runpy
fmt=runpy.run_path(str(ROOT/'scripts/build/font-format.py'))
if a.force or not target.exists():
    data=ranges+yo
else:
    data=target.read_bytes()
    if len(data)==1280:data+=yo
if not data.startswith(fmt['MAGIC']):data=fmt['wrap'](data)
fmt['parse'](data)
if not target.exists() or target.read_bytes()!=data:target.write_bytes(data)
print(f'{target.name}: {len(data)} bytes, MXFN v1; FONT.FNT preserved')
