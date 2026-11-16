"""Byte-exact window monitor build and provenance of the reused 8x8 font."""
from pathlib import Path
import hashlib

root = Path(__file__).resolve().parents[2]
original = (root / 'source/monitor_win/originals/monitor_win.rom').read_bytes()
assert hashlib.sha256(original).hexdigest() == '841a6db4bc330aa43514bcba98c02afd92db13f98670b435788adb6135bd7bac'
assert len(original) == 8192
assert (root / 'bin/MONWIN.mon').read_bytes() == original
# Modern Fast8 uses the separately edited KOI8-R font; the ROM stays original.
print('MONWIN: 8192 identical historical bytes')
