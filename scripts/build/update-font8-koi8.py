"""Derive the editable 8x8 KOI8-R font from the preserved window-monitor ROM."""
from pathlib import Path
import argparse,runpy
R=Path(__file__).resolve().parents[2]

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
    # Match the broad, two-pixel strokes of the original e/E and hard sign.
    font[0xa3*8:0xa4*8]=bytes.fromhex('6c 00 78 cc fc c0 78 00')
    font[0xb3*8:0xb4*8]=bytes.fromhex('6c 00 fe 60 7c 60 fe 00')
    font[0xff*8:0x100*8]=bytes.fromhex('e0 60 60 7c 66 66 7c 00')
    assert font[0x20*8:0x80*8]==rom[0x1900:0x1c00]
    assert font[0xc0*8:0xff*8]==rom[0x1e00:0x1ff8]
    assert font[0x9a*8:0x9b*8]==bytes(8) # non-breaking space
    assert font[0x8d*8:0x8e*8]==bytes([255]*8) # full block
    # Text has one blank left column; graphics keep full-width joins.
    for code in list(range(0x80))+list(range(0xc0,0x100))+[0xa3,0xb3]:
        font[code*8:code*8+8]=bytes(b>>1 for b in font[code*8:code*8+8])
    font[0x09*8:0x0a*8]=bytes.fromhex('00 11 09 7d 09 11 01 00')
    return bytes(font)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--write',action='store_true');args=parser.parse_args()
    target=R/'assets/system-rom/fonts/8x8/Win8.pf';font=build()
    if args.write:target.write_bytes(font)
    else:assert target.read_bytes()==font,'Run with --write to update the font'
    print('8x8 KOI8-R: 256 glyphs, graphics 80-BF, YO A3/B3, hard sign DF/FF PASS')
