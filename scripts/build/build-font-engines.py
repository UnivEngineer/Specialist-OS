"""Build fixed-slot font modules, with their ready-to-use embedded fonts."""
from pathlib import Path
import os
import runpy
import subprocess
import sys

ROOT = Path(__file__).absolute().parents[2]


def build_koi7(font, out):
    glyphs = [bytes(b & 63 for b in font[code*8:code*8+8]) for code in range(256)]
    glyphs[0x60:0x7F] = glyphs[0xE0:0xFF]
    glyphs[0x7F] = bytes(8)
    glyphs = glyphs[:128]

    def plane(row, shift):
        return bytes(((g[row] << shift) | (g[row] >> (6-shift))) & 255 for g in glyphs)

    split_rows = (0, 4, 1, 5, 2, 6, 3, 7)
    raw = b''.join(plane(row, 0) for row in split_rows)
    packed = b''.join(plane(row, shift) for shift in (2, 4, 6) for row in split_rows)
    separated = (lambda b: b << 2, lambda b: b >> 4, lambda b: (b << 4) & 255,
                 lambda b: b >> 2, lambda b: (b << 6) & 255, lambda b: b)

    def pair(left, right):
        return b''.join(bytes(separated[p](g[row]) for g in glyphs)
                        for row in range(8) for p in (left, right))

    (out/'koi7-rotate.bin').write_bytes(raw)
    (out/'koi7-packed.bin').write_bytes(packed)
    (out/'koi7-hybrid.bin').write_bytes(pair(3, 2)+raw)
    (out/'koi7-fast.bin').write_bytes(pair(0, 1)+pair(3, 2)+pair(4, 5))


def build():
    cpu_helpers = runpy.run_path(str(ROOT/'scripts/images/cpu.py'))
    cpu_format = cpu_helpers['format_cpu']
    def cpu_fields(path): return cpu_helpers['fields_from_symbols'](path.read_text('utf-8'))
    out = ROOT / 'bin/fonts'
    out.mkdir(parents=True, exist_ok=True)
    font = (ROOT / 'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
    assert len(font) == 2048
    paged = bytes(font[c * 8 + y] & 63 for y in range(8) for c in range(256))
    (out / 'paged-font.bin').write_bytes(paged)
    (out / 'packed-font.bin').write_bytes(bytes(((b << 6) | b) & 255 for b in paged))
    build_koi7(font, out)
    rotate_rows = ['-DFONT_MODE=0', '-DFONT_ROTATE_ROWS']
    rotate_columns = ['-DFONT_MODE=0', '-DFONT_ROTATE_COL4']
    for name, defines in [('Rotate', rotate_columns), ('Packed6K', ['-DFONT_MODE=1']), ('Fast12K', ['-DFONT_MODE=2'])]:
        subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        *defines, f'--raw={out / name}.i80',
                        f'--sym={out / name}.sym', f'--lst={out / name}.lst',
                        (ROOT / 'source/font/ENGINE.asm').as_posix()], check=True)
        (out / f'{name}.cpu').write_text(cpu_format(0xD800,flags=4,fields=cpu_fields(out/f'{name}.sym')), encoding='ascii')

    prerender = runpy.run_path(str(ROOT / 'scripts/build/build-font-prerender.py'))
    for name, mode in [('Packed6K', 'packed'), ('Fast12K', 'full')]:
        (out / f'{name}.pre').write_bytes(prerender['build'](font, mode))

    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    f'--raw={out / "Init6.i80"}', f'--sym={out / "Init6.sym"}',
                    f'--lst={out / "Init6.lst"}', (ROOT / 'source/font/INIT6.asm').as_posix()], check=True)
    (out / 'Init6.cpu').write_text(cpu_format(0x5000,flags=4,fields=cpu_fields(out/'Init6.sym')), encoding='ascii')

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
                        f'--lst={out / name}.lst', (ROOT / f'source/font/{source}.asm').as_posix()], check=True)
        (out / f'{name}.cpu').write_text(cpu_format(int(address,16),flags=4,fields=cpu_fields(out/f'{name}.sym')), encoding='ascii')

    win = (ROOT / 'assets/system-rom/fonts/8x8/Win8.pf').read_bytes()
    assert len(win) == 2048
    (out / 'win8-paged.bin').write_bytes(bytes(win[c*8+y] for y in range(8) for c in range(256)))
    font4 = (ROOT / 'assets/system-rom/fonts/4x8/Cpm4.pf').read_bytes()
    assert len(font4) == 2048 and all(b < 16 for b in font4)
    font4_pages = bytes(font4[c*8+y] for y in range(8) for c in range(256))
    (out / 'cpm4-paged.bin').write_bytes(bytes(b * 17 for b in font4_pages))
    (out / 'cpm4-compact.bin').write_bytes(bytes(
        font4[c * 8 + y] * 16 + font4[(c + 1) * 8 + y]
        for c in range(0, 256, 2) for y in range(8)))
    (out / 'cpm4-left.bin').write_bytes(bytes(b * 16 for b in font4_pages))
    (out / 'Cpm4Fast.pre').write_bytes(font4_pages)
    graph = ['-DCELL_WIDTH=8', '-DFONT_GRAPHIC_X']
    quad = [*graph, '-DFONT_GRAPHIC_PAIR', '-DFONT_GRAPHIC_QUAD']
    aligned_modules = [
        ('Fast8', 'ALIGNED', 'D800', ['-DCELL_WIDTH=8']),
        ('Fast8Row', 'ALIGNED', 'D800', ['-DCELL_WIDTH=8', '-DFONT_ALIGNED_ROWS']),
        ('Graph8', 'ALIGNED', 'D800', graph),
        ('Graph8x2', 'ALIGNED', 'D800', [*graph, '-DFONT_GRAPHIC_PAIR']),
        ('Graph8x4', 'ALIGNED', 'D800', quad),
        ('GraphP8K', 'ALIGNED', 'D800', [*quad, '-DGRAPH_PRE_MODE=1']),
        ('Graph14K', 'ALIGNED', 'D800', [*quad, '-DGRAPH_PRE_MODE=2']),
        ('Cpm4', 'ALIGNED', 'D800', ['-DCELL_WIDTH=4']),
        ('Cpm4Compact', 'ALIGNED', 'D800', ['-DCELL_WIDTH=4', '-DFONT4_COMPACT']),
        ('Cpm4Fast', 'ALIGNED', 'D800', ['-DCELL_WIDTH=4', '-DFONT4_FAST']),
        ('InitA', 'INITA', '5000', []),
        ('Init7', 'INIT7', '5000', []),
        ('K7Rotate', 'KOI7', 'D800', ['-DK7_MODE=0']),
        ('K7Packed', 'KOI7', 'D800', ['-DK7_MODE=1']),
        ('K7Hybrid', 'KOI7', 'D800', ['-DK7_MODE=2']),
    ]
    for name, source, address, defines in aligned_modules:
        subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow', *defines,
                        f'--raw={out / name}.i80', f'--sym={out / name}.sym',
                        f'--lst={out / name}.lst', (ROOT / f'source/font/{source}.asm').as_posix()], check=True)
        (out / f'{name}.cpu').write_text(cpu_format(int(address,16),flags=4,fields=cpu_fields(out/f'{name}.sym')), encoding='ascii')

    for name, mode in [('GraphP8K', 'graph-packed'), ('Graph14K', 'graph-full')]:
        (out / f'{name}.pre').write_bytes(prerender['build'](win, mode))

    # FontBen uses the same frontends, relocated to MAIN. Experimental kernels
    # have their own build switches so installed production fonts stay untouched.
    benchmark_modules = [
        ('FontRot', 'ENGINE', ['-DFONT_MODE=0']),
        ('RotRows', 'ENGINE', [*rotate_rows, '-DFONT_BENCH_STREAM']),
        ('Rot6Col4', 'ENGINE', rotate_columns),
        ('FontPack', 'ENGINE', ['-DFONT_MODE=1', '-DFONT_BENCH_STREAM']),
        ('FontFull', 'ENGINE', ['-DFONT_MODE=2', '-DFONT_BENCH_STREAM']),
        ('FontKoi', 'COMPACT', []),
        ('Font8', 'ALIGNED', ['-DCELL_WIDTH=8']),
        ('Rot8Quad', 'ALIGNED', ['-DCELL_WIDTH=8', '-DFONT_GRAPHIC_X',
                                 '-DFONT_GRAPHIC_PAIR', '-DFONT_GRAPHIC_QUAD']),
        ('Rot8Strm', 'ALIGNED', ['-DCELL_WIDTH=8', '-DFONT_GRAPHIC_X']),
        ('Font4', 'ALIGNED', ['-DCELL_WIDTH=4', '-DFONT_BENCH_STREAM']),
        ('Font4Fst', 'ALIGNED', ['-DCELL_WIDTH=4', '-DFONT_BENCH_STREAM', '-DFONT4_FAST']),
        ('Font4Cmp', 'ALIGNED', ['-DCELL_WIDTH=4', '-DFONT_BENCH_STREAM', '-DFONT4_COMPACT']),
        ('K7Fast', 'KOI7', ['-DK7_MODE=3']),
    ]
    for name, source, defines in benchmark_modules:
        subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow','-DFONT_BENCH_MAIN',*defines,
                        f'--raw={out / name}.i80', f'--sym={out / name}.sym',
                        f'--lst={out / name}.lst', (ROOT / f'source/font/{source}.asm').as_posix()], check=True)
        (out / f'{name}.cpu').write_text(cpu_format(0x2000,flags=4,fields=cpu_fields(out/f'{name}.sym')), encoding='ascii')


if __name__ == '__main__':
    from build_cache import python_build

    os.chdir(ROOT)
    sources = ('ENGINE', 'INIT6', 'COMPACT', 'INITKOI', 'ALIGNED', 'INITA', 'INIT7', 'KOI7')
    sys.exit(python_build(__file__, 'Font engines', build,
                          asm=(f'source/font/{source}.asm' for source in sources),
                          inputs=('assets/system-rom/fonts/6x8/KOI8.pf', 'assets/system-rom/fonts/8x8/Win8.pf',
                                  'assets/system-rom/fonts/4x8/Cpm4.pf',
                                  'scripts/build/build-font-prerender.py', 'scripts/build/build-compact-font.py',
                                  'scripts/build/font-format.py', 'scripts/images/cpu.py'),
                          exclude=('bin/fonts',),
                          input_content=True,
                          outputs=('bin/fonts/Compact.pf',), scan_outputs=('bin/fonts',)))
