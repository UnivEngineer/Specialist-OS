"""Создание компактного системного шрифта из полного FONT.FNT."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
assert len(source) == 2048
result = source[0x20*8:0x80*8] + source[0xc0*8:0x100*8]
target = ROOT / 'assets/system-rom/FONTKOI.FNT'
if not target.exists() or target.read_bytes() != result:
    target.write_bytes(result)
print(f'{target.name}: {len(result)} bytes; FONT.FNT preserved')
