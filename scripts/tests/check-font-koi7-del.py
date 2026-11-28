"""8080: DEL в KOI-7R, обратное переключение и оригинальная процедура StarTrek."""
from pathlib import Path
import runpy
import tempfile

ROOT = Path(__file__).resolve().parents[2]
runner = ROOT / 'source-apps/fontbench/check-engines.py'
model = {'__file__': str(runner)}
exec(compile(runner.read_text(encoding='utf8').split('cases=')[0], str(runner), 'exec'), model)
bs, base, run, symbols = [model[key] for key in ('bs','base','run','symbols')]
font_bin = ROOT / 'bin/fonts'
cursor = bs['mon2_vars.cursorY']
inverse_flag = bs['mon2_vars.inverse']
encoding = bs['FONT_ENGINE_ENCODING']
fixtures = {
    4: ('Cpm4','Cpm4Fast','Cpm4Compact'),
    6: ('Rotate','Packed6K','Fast12K','Compact'),
    8: ('Fast8','Fast8Row','Graph8x4','GraphP8K','Graph14K'),
}


def seed(name):
    memory = bytearray(base)
    body = (font_bin / f'{name}.i80').read_bytes()
    memory[0xd800:0xd800+len(body)] = body
    lower_size = int.from_bytes(body[44:46], 'little')
    lower_base = 0x5000 if lower_size else 0
    if lower_size:
        lower = (font_bin / f'{name}.pre').read_bytes()
        assert len(lower) == lower_size
        memory[lower_base:lower_base+lower_size] = lower
    memory, _, _ = run(memory, bs['FONT_ENGINE_INIT'], de=lower_base)
    return memory, lower_base, lower_size


def original_glyphs(width, name):
    if name == 'Compact':
        data = (font_bin/'compact-font.bin').read_bytes()
        lookup = (font_bin/'compact-map.bin').read_bytes()
        return tuple(data[i*8:i*8+8] for i in lookup)
    folder = ROOT/'assets/system-rom/fonts'
    filename = {4: '4x8/Cpm4.pf', 6: '6x8/KOI8.pf', 8: '8x8/Win8.pf'}[width]
    data = (folder/filename).read_bytes()
    return tuple(data[i:i+8] for i in range(0,len(data),8))


def koi7_glyphs(original):
    glyphs = list(original)
    glyphs[0x60:0x7f], glyphs[0xe0:0xff] = original[0xe0:0xff], original[0x60:0x7f]
    glyphs[0x7f] = bytes(8)
    return glyphs


def expected_pixels(text, glyphs, x, width, inverse):
    pixels = bytearray([0xa5]) * 12288
    for code in text:
        for row, byte in enumerate(glyphs[code]):
            for bit in range(width):
                at = (x+bit)//8*256 + row
                mask = 128>>((x+bit)%8)
                on = bool(byte & (1<<(width-1-bit))) ^ inverse
                pixels[at] = (pixels[at]&~mask) | (mask if on else 0)
        x += width
    return pixels


def check_pixels(memory, glyphs, width, name, koi):
    text = bytes([0x7e,0x7f,0xfe,0xff,0x7f,0x60,0xe0,0x80])
    phases = range(4) if name.startswith('Graph') or width==6 else (0,2) if width==4 else (0,)
    for x in phases:
        for inverse in (False,True):
            seed = bytearray(memory)
            seed[cursor:cursor+2] = bytes([7,x])
            seed[inverse_flag:inverse_flag+2] = bytes([255*inverse])*2
            seed[0x9000:0xc000] = bytes([0xa5])*12288
            seed[0x8600:0x8600+len(text)+1] = text + b'\0'
            for api, output in (('mon2_printCharOld', b'\x7f'), ('FONT_ENGINE_RAW', b'\x7f'),
                                ('mon2_printStringOld', text), ('FONT_ENGINE_RAWZ', text),
                                ('FONT_ENGINE_LIM', text)):
                rendered, _, _ = run(seed,bs[api],af=0x7f00,bc=0x7f if len(output)==1 else len(text),hl=0x8600)
                assert rendered[0x9000:0xc000] == expected_pixels(output,glyphs,x*2,width,inverse), (name,koi,x,inverse,api)
                assert rendered[cursor:cursor+2] == bytes([7,x+len(output)*width//2]), (name,api,'cursor')


with tempfile.TemporaryDirectory(prefix='koi7-del-') as temporary:
    model['O'] = Path(temporary)
    for width, names in fixtures.items():
        for name in names:
            original, lo, size = seed(name)
            before = original_glyphs(width,name)
            module = symbols(f'fonts/{name}')
            start, end = module['fontData'], module['imageEnd']
            if name == 'Compact':
                start = module['glyphMap']
            regions = [(start,end), (lo,lo+size)]
            switched, registers, _ = run(original,encoding,af=0x0197)
            assert registers[:2] == bytes([0x97,1]) and switched[bs['bios_upper.koi7']]==255, name
            repeated, _, _ = run(switched,encoding,af=0xff00)
            assert all(repeated[a:b]==switched[a:b] for a,b in regions), (name,'KOI7 idempotence')
            check_pixels(repeated,koi7_glyphs(before),width,name,7)
            restored, _, _ = run(repeated,encoding,af=0)
            assert restored[bs['bios_upper.koi7']]==0
            assert all(restored[a:b]==original[a:b] for a,b in regions), (name,'KOI8 restoration')
            check_pixels(restored,before,width,name,8)
            attached, _, _ = run(switched,bs['FONT_ENGINE_INIT'],de=lo)
            assert all(attached[a:b]==original[a:b] for a,b in regions), (name,'reattach')
            print(name, 'DEL, FF, KOI8, batch/scalar and inverse PASS', flush=True)

    rks = (ROOT/'assets/games/families/startrek/originals/StarTrek-std.rks').read_bytes()[4:-2]
    assert rks[0x198d:0x1998] == bytes.fromhex('0e08cd37c00e7fcd37c0c9')
    memory, _, _ = seed('Rotate')
    memory[:len(rks)] = rks
    memory, _, _ = run(memory,encoding,af=0x100)
    memory[cursor:cursor+2] = bytes([7,3])
    memory[0x9000:0xc000] = bytes([255])*12288
    erased, _, _ = run(memory,0x198d)
    assert erased[0x9000:0x9008]==bytes([3])*8
    assert erased[0x9008:0xc000]==memory[0x9008:0xc000]
    assert erased[cursor:cursor+2]==bytes([7,3])
    print('StarTrek original C037h erase adapter, neighboring pixels and cursor PASS')
