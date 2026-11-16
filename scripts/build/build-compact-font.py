"""Derive Compact's selected KOI8 glyphs from the common editable 6x8 font."""
from pathlib import Path
import runpy
ROOT = Path(__file__).resolve().parents[2]
source = bytearray((ROOT / 'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes())
assert len(source) == 2048
# DEL is the visible placeholder for missing glyphs in Compact.
source[0x7f*8:0x80*8] = bytes([63,33,33,33,33,33,63,0])
payload = source[0x20*8:0x80*8] + source[0xc0*8:0x100*8] + source[0xa3*8:0xa4*8] + source[0xb3*8:0xb4*8]
fmt = runpy.run_path(str(ROOT / 'scripts/build/font-format.py'))
data = fmt['wrap'](payload)
fmt['parse'](data)
target = ROOT / 'bin/fonts/Compact.pf'
target.parent.mkdir(parents=True, exist_ok=True)
if not target.exists() or target.read_bytes() != data: target.write_bytes(data)
print('Compact: 162 glyphs derived from KOI8.pf; DEL placeholder retained')
