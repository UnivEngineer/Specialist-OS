"""Готовые сдвиги всех глифов для автономного FONTBEN; вне измерения."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
font = (root / 'assets/system-rom/FONT.FNT').read_bytes()
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
