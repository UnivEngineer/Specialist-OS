"""Patch FONT.FNT and FONT_IT.FNT: KOI8-R graphics and visible raw controls.

Rows run top to bottom. Bits 5..0 run left to right; bits 7..6 stay zero.
Run with --write to apply, without arguments to check the binary asset.
"""
from pathlib import Path
import argparse

ROOT = Path(__file__).resolve().parents[2]

# IBM PC style pictures, with visible NUL and explicit LF/CR labels.
# KOI8-R does not assign printable characters to control codes 00h-1Fh.
# LF/CR: first letter at upper left, second at lower right (Control Pictures).
CONTROLS = """
00 0e 13 15 15 19 0e 00 00
01 0e 11 1b 11 15 0e 00 00
02 0e 1f 15 1f 1b 0e 00 00
03 00 0a 1f 1f 0e 04 00 00
04 00 04 0e 1f 0e 04 00 00
05 04 0e 04 1f 1f 04 0e 00
06 04 0e 1f 1f 04 04 0e 00
07 00 00 00 0c 0c 00 00 00
08 3f 3f 3f 33 33 3f 3f 3f
09 00 0e 11 11 11 0e 00 00
0a 20 20 38 00 07 04 06 04
0b 07 03 05 0c 12 12 0c 00
0c 0e 11 11 0e 04 0e 04 00
0d 18 20 18 00 06 05 06 05
0e 03 0d 09 09 0b 1b 18 00
0f 04 15 0e 1b 0e 15 04 00
10 10 18 1c 1e 1c 18 10 00
11 02 06 0e 1e 0e 06 02 00
12 04 0e 15 04 15 0e 04 00
13 0a 0a 0a 0a 00 0a 00 00
14 0f 1d 1d 0d 05 05 05 00
15 0e 10 0c 12 0c 02 1c 00
16 00 00 00 00 1f 1f 00 00
17 04 0e 15 04 0e 04 1f 00
18 04 0e 15 04 04 04 04 00
19 04 04 04 04 15 0e 04 00
1a 00 04 02 1f 02 04 00 00
1b 00 04 08 1f 08 04 00 00
1c 00 00 10 10 10 1f 00 00
1d 00 00 12 3f 12 00 00 00
1e 00 04 0e 1f 00 00 00 00
1f 00 00 1f 0e 04 00 00 00
"""

# Eight rows per character. Lines reach the edges of the complete 6x8 cell.
# Single strokes: x=2, y=3; double strokes: x=1/3, y=2/4.
ROWS = """
80 00 00 00 3f 00 00 00 00
81 08 08 08 08 08 08 08 08
82 00 00 00 0f 08 08 08 08
83 00 00 00 38 08 08 08 08
84 08 08 08 0f 00 00 00 00
85 08 08 08 38 00 00 00 00
86 08 08 08 0f 08 08 08 08
87 08 08 08 38 08 08 08 08
88 00 00 00 3f 08 08 08 08
89 08 08 08 3f 00 00 00 00
8a 08 08 08 3f 08 08 08 08
8b 3f 3f 3f 3f 00 00 00 00
8c 00 00 00 00 3f 3f 3f 3f
8d 3f 3f 3f 3f 3f 3f 3f 3f
8e 38 38 38 38 38 38 38 38
8f 07 07 07 07 07 07 07 07
90 22 00 08 00 22 00 08 00
91 2a 15 2a 15 2a 15 2a 15
92 1d 3f 37 3f 1d 3f 37 3f
93 06 09 08 08 08 08 08 08
94 00 00 1e 1e 1e 1e 00 00
95 00 00 0c 1e 1e 0c 00 00
96 07 04 04 04 14 0c 04 00
97 00 00 0a 14 00 0a 14 00
98 02 04 08 10 08 04 1e 00
99 10 08 04 02 04 08 1e 00
9a 00 00 00 00 00 00 00 00
9b 08 08 08 08 08 08 24 18
9c 0c 12 12 0c 00 00 00 00
9d 0c 12 04 08 1e 00 00 00
9e 00 00 00 0c 0c 00 00 00
9f 00 04 00 1f 00 04 00 00
a0 00 00 3f 00 3f 00 00 00
a1 14 14 14 14 14 14 14 14
a2 00 00 0f 08 0f 08 08 08
a3 0a 00 0e 11 1f 10 0e 00
a4 00 00 00 1f 14 14 14 14
a5 00 00 1f 10 17 14 14 14
a6 00 00 38 08 38 08 08 08
a7 00 00 00 3c 14 14 14 14
a8 00 00 3c 04 34 14 14 14
a9 08 08 0f 08 0f 00 00 00
aa 14 14 14 1f 00 00 00 00
ab 14 14 17 10 1f 00 00 00
ac 08 08 38 08 38 00 00 00
ad 14 14 14 3c 00 00 00 00
ae 14 14 34 04 3c 00 00 00
af 08 08 0f 08 0f 08 08 08
b0 14 14 14 17 14 14 14 14
b1 14 14 17 10 17 14 14 14
b2 08 08 38 08 38 08 08 08
b3 0a 1f 10 1e 10 10 1f 00
b4 14 14 14 3c 14 14 14 14
b5 14 14 34 04 34 14 14 14
b6 00 00 3f 08 3f 08 08 08
b7 00 00 00 3f 14 14 14 14
b8 00 00 3f 00 37 14 14 14
b9 08 08 3f 08 3f 00 00 00
ba 14 14 14 3f 00 00 00 00
bb 14 14 37 00 3f 00 00 00
bc 08 08 3f 08 3f 08 08 08
bd 14 14 14 3f 14 14 14 14
be 14 14 37 00 37 14 14 14
bf 1e 21 2d 29 2d 21 1e 00
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    for name in ('FONT.FNT', 'FONT_IT.FNT'):
        path = ROOT / 'assets/system-rom' / name
        original = path.read_bytes()
        assert len(original) == 2048
        font = bytearray(original)
        for first, last, table in ((0, 0x1f, CONTROLS), (0x80, 0xbf, ROWS)):
            lines = table.strip().splitlines()
            assert len(lines) == last-first+1
            for code, line in enumerate(lines, first):
                values = bytes.fromhex(line)
                assert values[0] == code and len(values) == 9
                assert all(row < 64 for row in values[1:])
                font[code*8:code*8+8] = values[1:]
        if name == 'FONT_IT.FNT':
            # Match this font's own lowercase e and uppercase E.
            font[0xa3*8:0xa4*8] = bytes.fromhex('0a 00 0e 11 1f 10 0e 00')
            font[0xb3*8:0xb4*8] = bytes.fromhex('0a 1f 10 1e 10 10 1f 00')
        assert font[0x20*8:0x80*8] == original[0x20*8:0x80*8]
        assert font[0xc0*8:] == original[0xc0*8:]
        assert all(row < 64 for row in font)
        if args.write:
            path.write_bytes(font)
        else:
            assert font == original, f'Run with --write to update {name}'
        print(f'{name}: visible 00h-1Fh, KOI8-R 80h-BFh, right-aligned 6x8, 2048 bytes')


if __name__ == '__main__':
    main()
