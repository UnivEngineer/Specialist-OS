"""Pixel and ABI checks for the explicit ten-row RAW mode."""
from pathlib import Path
import runpy
import tempfile

ROOT = Path(__file__).resolve().parents[2]
runner = ROOT / 'source-apps/fontbench/check-engines.py'
model = {'__file__': str(runner)}
exec(compile(runner.read_text(encoding='utf-8').split('cases=')[0], str(runner), 'exec'), model)
bios, run, base, word = [model[k] for k in ('bs', 'run', 'base', 'w')]

variants = (
    ('Cpm4', 4, '4x8/Cpm4.pf'),
    ('Rotate', 6, '6x8/KOI8.pf'),
    ('Packed6K', 6, '6x8/KOI8.pf'),
    ('Fast12K', 6, '6x8/KOI8.pf'),
    ('Compact', 6, '6x8/KOI8.pf'),
    ('Fast8', 8, '8x8/Win8.pf'),
    ('Graph8x4', 8, '8x8/Win8.pf'),
    ('GraphP8K', 8, '8x8/Win8.pf'),
    ('Graph14K', 8, '8x8/Win8.pf'),
)
font_dir = ROOT / 'bin/fonts'
start = 0xD800
screen = 0x9000
cursor = bios['bios_vars.cursorY']
inverse_flag = bios['bios_vars.inverse']


def put_pixel(memory, x, y, value):
    if 0 <= x < 384 and 0 <= y < 256:
        address = screen + (x // 8) * 256 + y
        mask = 128 >> (x % 8)
        memory[address] = (memory[address] & ~mask) | (mask if value else 0)


def draw_cell(memory, font, name, width, code, pos, y, inverse):
    glyph = font[code * 8:code * 8 + 8]
    extend = name != 'Compact' and 0x2500 <= ord(bytes([code]).decode('koi8-r')) <= 0x259F
    bitmap = bytes([glyph[0] if extend else 0]) + glyph + bytes([glyph[-1] if extend else 0])
    for row, bits in enumerate(bitmap, y - 8):
        for bit in range(width):
            value = bool(bits & (1 << (width - bit - 1))) ^ inverse
            put_pixel(memory, pos * 2 + bit, row, value)


def oracle(memory, font, name, width, text, pos, y, inverse):
    for code in text:
        if pos >= (190 if width == 6 else 192):
            pos, y = 0, y + 10
        if name == 'Fast8' or width == 4:
            pos &= ~(width // 2 - 1)
        draw_cell(memory, font, name, width, code, pos, y, inverse)
        pos += width // 2
        if pos >= 192:
            pos, y = 0, y + 10
    return bytes([y, pos])


def seed(name):
    m = bytearray(base)
    binary = (font_dir / (name + '.i80')).read_bytes()
    m[start:start + len(binary)] = binary
    size = int.from_bytes(binary[44:46], 'little')
    lower = 0x7000 - size if size else 0
    if size:
        m[lower:lower + size] = (font_dir / (name + '.pre')).read_bytes()
    m, _, _ = run(m, bios['FONT_ENGINE_INIT'], de=lower)
    return m


with tempfile.TemporaryDirectory(prefix='font-pseudo-') as work:
    model['O'] = Path(work)
    for name, width, file in variants:
        font = (ROOT / 'assets/system-rom/fonts' / file).read_bytes()
        if name == 'Compact':
            glyphs = (font_dir / 'compact-font.bin').read_bytes()
            indices = (font_dir / 'compact-map.bin').read_bytes()
            font = b''.join(glyphs[i * 8:i * 8 + 8] for i in indices)
        initial = seed(name)
        for inv in (0, 1, 255):
            for x, y, text in (
                (0, 7, b'\x80A\xb0\x91\x92\xa3\xb3\xbf\x93'),
                (1, 17, b'\x81\x82 AA\x8d\x90'),
                (2, 30, b'\x85\x93\xb3\xa4\0\r\n'),
                (3, 255, b'\x80A\xb0'),
                (188, 27, b'\x81\xb3\xb0\x91'),
                (0, 18, b'   \x81A.'),
                (0, 8, bytes(range(256)) + b'A\x81\xb3\x80'*11),
                (191, 37, b'\x86'),
                (11, 59, b''),
            ):
                m = bytearray(initial)
                m[screen:0xc000] = bytes([0xA5]) * 12288
                m[0x200:0x200 + len(text)] = text
                m[cursor:cursor + 2] = bytes([y, x])
                m[inverse_flag] = inv
                expected = bytearray(m)
                position = oracle(expected, font, name, width, text, x, y, bool(inv))
                result, regs, _ = run(m, bios['bios_printStringRawPseudo'], af=0x0097, hl=0x200, bc=len(text), de=0x2468)
                assert result[screen:0xc000] == expected[screen:0xc000], (
                    name, inv, x, y, [(hex(i), result[i], expected[i])
                                      for i in range(screen, 0xc000) if result[i] != expected[i]][:24])
                assert result[cursor:cursor + 2] == position, (name, inv, x, y, 'cursor')
                assert regs[:6] == word(0x0097) + word(len(text)) + word(0x200 + len(text)), (name, 'AF/BC/HL')
                assert regs[12:14] == word(0x2468), (name, 'DE')
            # Exactly four cells reach the corner. The complete 10-row
            # bitmap must be in VRAM before the regular BIOS scroll.
            text = b'\x81\x8aA\x81'
            y = 247
            x = 192 - len(text) * (width // 2)
            m = bytearray(initial)
            m[screen:0xc000] = bytes([0xA5]) * 12288
            m[0x200:0x200 + len(text)] = text
            m[cursor:cursor + 2] = bytes([y, x])
            m[inverse_flag] = inv
            expected = bytearray(m)
            for i, code in enumerate(text):
                draw_cell(expected, font, name, width, code, x + i * (width // 2), y, bool(inv))
            expected[cursor:cursor + 2] = bytes([y, 192])
            expected, _, _ = run(expected, bios['bios_fontNewLine'], hl=(192 << 8) | y)
            result, _, _ = run(m, bios['bios_printStringRawPseudo'], hl=0x200, bc=len(text))
            assert result[screen:0xc000] == expected[screen:0xc000], (name, inv, 'bottom-right scroll')
            assert result[cursor:cursor + 2] == expected[cursor:cursor + 2], (name, inv, 'scroll cursor')
        print(name, 'margins, adjacent pixels, wrap and public registers PASS', flush=True)

