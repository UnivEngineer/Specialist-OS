"""Derive the editable 8x8 KOI8-R font from the preserved window-monitor ROM."""
from pathlib import Path
import argparse,runpy
R=Path(__file__).resolve().parents[2]

# Hand-drawn rows in their final 8-pixel alignment.
ROWS_8X8 = """
00 00 1e 23 25 29 31 1e 00
01 3c 42 a5 81 a5 99 42 3c
02 3c 7e db ff db e7 7e 3c
07 00 00 18 3c 3c 18 00 00
08 ff ff e7 c3 c3 e7 ff ff
09 01 19 0d 7f 0d 19 01 00
0a ff c3 99 bd bd 99 c3 ff
0f 18 5a 3c e7 e7 3c 5a 18
10 c0 f0 fc ff fc f0 c0 00
11 03 0f 3f ff 3f 0f 03 00
1d 00 24 66 ff 66 24 00 00
1e 00 18 3c 7e ff ff 00 00
1f 00 ff ff 7e 3c 18 00 00
40 3e 63 6f 6b 6e 60 3e 00
7c 0c 0c 0c 0c 0c 0c 0c 00
90 44 00 10 00 44 00 10 00
91 55 aa 55 aa 55 aa 55 aa
92 bb ff ef ff bb ff ef ff
93 0e 1b 18 18 18 18 18 18
97 00 00 3b 6e 00 3b 6e 00
98 06 0c 18 30 3e 00 3e 00
99 30 18 0c 06 3e 00 3e 00
9b 18 18 18 18 18 18 d8 70
b3 36 7f 31 3c 30 31 7f 00
b9 10 10 ff 00 ff 00 00 00
bf 1c 22 5d 51 5d 22 1c 00
f9 63 63 63 7b 6f 6f 7b 00
"""

def build():
    rom=(R/'source/monitor_win/originals/monitor_win.rom').read_bytes()
    font=bytearray(rom[0x1800:0x2000])
    rows=runpy.run_path(str(R/'scripts/build/update-font-koi8.py'))['ROWS']
    # Keep the middle four columns; extend both edge columns to 8 pixels.
    # Thus horizontal rules/blocks reach both cell edges; vertical/double
    # lines retain separate columns, rather than scaling a bitmap image.
    def widen(b):
        return ((b&0x20)<<2)|((b&0x3f)<<1)|(b&1)
    for line in rows.strip().splitlines():
        data=bytes.fromhex(line);code=data[0]
        font[code*8:code*8+8]=bytes(widen(b) for b in data[1:])
    # Match the broad, two-pixel strokes of the original lowercase e and hard sign.
    font[0xa3*8:0xa4*8]=bytes.fromhex('6c 00 78 cc fc c0 78 00')
    font[0xff*8:0x100*8]=bytes.fromhex('e0 60 60 7c 66 66 7c 00')
    assert font[0x20*8:0x80*8]==rom[0x1900:0x1c00]
    assert font[0xc0*8:0xff*8]==rom[0x1e00:0x1ff8]
    assert font[0x9a*8:0x9b*8]==bytes(8) # non-breaking space
    assert font[0x8d*8:0x8e*8]==bytes([255]*8) # full block
    # Start with a blank left column in text glyphs and full-width graphics.
    for code in list(range(0x80))+list(range(0xc0,0x100))+[0xa3,0xb3]:
        font[code*8:code*8+8]=bytes(b>>1 for b in font[code*8:code*8+8])
    for line in ROWS_8X8.strip().splitlines():
        code, *rasters = bytes.fromhex(line)
        assert len(rasters) == 8
        font[code*8:code*8+8]=bytes(rasters)
    return bytes(font)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--write',action='store_true');args=parser.parse_args()
    target=R/'assets/system-rom/fonts/8x8/Win8.pf';font=build()
    if args.write:target.write_bytes(font)
    else:assert target.read_bytes()==font,'Run with --write to update the font'
    print('8x8 KOI8-R: 256 glyphs, graphics 80-BF, YO A3/B3, hard sign DF/FF PASS')
