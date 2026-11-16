"""Build fixed-slot font modules, with their ready-to-use embedded fonts."""
from pathlib import Path
import subprocess
import runpy

ROOT = Path(__file__).resolve().parents[2]
out = ROOT / 'bin/fonts'
out.mkdir(parents=True, exist_ok=True)
font = (ROOT / 'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
assert len(font) == 2048
paged = bytes(font[c * 8 + y] & 63 for y in range(8) for c in range(256))
(out / 'paged-font.bin').write_bytes(paged)
(out / 'packed-font.bin').write_bytes(bytes(((b << 6) | b) & 255 for b in paged))
for name, packed in [('Rotate', 0), ('Packed6K', 1), ('Fast12K', 2)]:
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    f'-DFONT_MODE={packed}', f'--raw={out / name}.i80',
                    f'--sym={out / name}.sym', f'--lst={out / name}.lst',
                    str(ROOT / 'source/font/ENGINE.asm')], check=True)
    (out / f'{name}.cpu').write_text('D800 D800 04\n', encoding='ascii')

prerender = runpy.run_path(str(ROOT / 'scripts/build/build-font-prerender.py'))
for name, mode in [('Packed6K', 'packed'), ('Fast12K', 'full')]:
    (out / f'{name}.pre').write_bytes(prerender['build'](font, mode))

subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                f'--raw={out / "Init6.i80"}', f'--sym={out / "Init6.sym"}',
                f'--lst={out / "Init6.lst"}', str(ROOT / 'source/font/INIT6.asm')], check=True)
(out / 'Init6.cpu').write_text('5000 5000 04\n', encoding='ascii')

# Derive the compact selection from the common editable 6x8 source.
runpy.run_path(str(ROOT / 'scripts/build/build-compact-font.py'), run_name='__main__')
# The generated MXFN header is not resident.
fmt = runpy.run_path(str(ROOT / 'scripts/build/font-format.py'))
compact = (ROOT / 'bin/fonts/Compact.pf').read_bytes()
meta = fmt['parse'](compact)
codes = list(range(0x20, 0x80)) + list(range(0xc0, 0x100)) + [0xa3, 0xb3]
space = compact[meta['ranges'][1]:meta['ranges'][1]+8]
def glyph(code):
    if code in (0xa3, 0xb3):
        offset = meta['yo'][code == 0xb3]
    else:
        base = meta['ranges'][code >> 5]
        offset = base + (code & 31) * 8 if base else 0
    return compact[offset:offset+8] if offset else space
(out / 'compact-font.bin').write_bytes(bytes(b & 63 for c in codes for b in glyph(c)))
lookup = bytearray([codes.index(0x7f)] * 256)
for index, code in enumerate(codes): lookup[code] = index
(out / 'compact-map.bin').write_bytes(lookup)
(out / 'compact-codes.bin').write_bytes(bytes(codes))
for name, source, address in [('Compact', 'COMPACT', 'D800'), ('InitKoi', 'INITKOI', '5000')]:
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    f'--raw={out / name}.i80', f'--sym={out / name}.sym',
                    f'--lst={out / name}.lst', str(ROOT / f'source/font/{source}.asm')], check=True)
    (out / f'{name}.cpu').write_text(f'{address} {address} 04\n', encoding='ascii')

win = (ROOT / 'assets/system-rom/fonts/8x8/Win8.pf').read_bytes()
assert len(win) == 2048
(out / 'win8-paged.bin').write_bytes(bytes(win[c*8+y] for y in range(8) for c in range(256)))
font4 = (ROOT / 'assets/system-rom/fonts/4x8/Cpm4.pf').read_bytes()
assert len(font4) == 2048 and all(b < 16 for b in font4)
(out / 'cpm4-paged.bin').write_bytes(bytes(font4[c*8+y]*17 for y in range(8) for c in range(256)))
for name, source, address, defines in [('Fast8', 'ALIGNED', 'D800', ['-DCELL_WIDTH=8']), ('Cpm4', 'ALIGNED', 'D800', ['-DCELL_WIDTH=4']), ('InitA', 'INITA', '5000', [])]:
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow', *defines,
                    f'--raw={out / name}.i80', f'--sym={out / name}.sym',
                    f'--lst={out / name}.lst', str(ROOT / f'source/font/{source}.asm')], check=True)
    (out / f'{name}.cpu').write_text(f'{address} {address} 04\n', encoding='ascii')

# FontBen uses the production source, relocated to MAIN; no copied kernels.
for name, source, defines in [('FontRot','ENGINE',['-DFONT_MODE=0']),('FontPack','ENGINE',['-DFONT_MODE=1']),('FontFull','ENGINE',['-DFONT_MODE=2']),('FontKoi','COMPACT',[]),('Font8','ALIGNED',['-DCELL_WIDTH=8']),('Font4','ALIGNED',['-DCELL_WIDTH=4'])]:
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow','-DFONT_BENCH_MAIN',*defines,
                    f'--raw={out / name}.i80', f'--sym={out / name}.sym',
                    f'--lst={out / name}.lst', str(ROOT / f'source/font/{source}.asm')], check=True)
    (out / f'{name}.cpu').write_text('2000 2000 04\n', encoding='ascii')
