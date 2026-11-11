"""8080: три формата 4x8, все коды, пары, хвосты и сохранение соседних пикселей."""
from pathlib import Path
import argparse
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/font4-run/cpu')
    parser.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    s = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT / 'bin/FONTCOM.sym').read_text(), re.M)}
    program = (ROOT / 'bin/FONTCOM.i80').read_bytes()
    base = bytearray((i * 37 + (i >> 8) * 13) & 255 for i in range(65536))
    base[:len(program)] = program
    font = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
    font4 = (ROOT / 'assets/system-rom/FONT4CPM.bin').read_bytes()
    narrow = bytes(font4[y * 256 + c] & 15 for c in range(256) for y in range(8))
    word = lambda n: n.to_bytes(2, 'little')
    call = lambda n: b'\xcd' + word(s[n])
    base[s['renderFont8']:s['renderFont8'] + 2] = word(0x2800 // 8)

    def execute(m, code):
        code += b'\xc3\x80\x8b'
        m[0x8000:0x8000 + len(code)] = code
        (args.out / 'input.bin').write_bytes(m)
        subprocess.run([str(args.ticks), '-m8080', '-pc', '8000', '-end', '8b80', '-counter', '20000000', '-output', str(args.out / 'output.bin'), str(args.out / 'input.bin')], capture_output=True, check=True, timeout=20)
        return (args.out / 'output.bin').read_bytes()[:65536]

    checks = 0
    def render(m, chars, x, y, scalar=False, input_at=0x7c00):
        nonlocal checks
        m = bytearray(m)
        m[input_at:input_at + len(chars)] = chars
        m[s['bios_vars.cursorY']:s['bios_vars.cursorY'] + 2] = bytes((y, x // 2))
        count = len(chars)
        valid = (not scalar and count == 0) or (count <= 48 and x % 4 == 0 and x + count * 4 <= 384 and y >= 7)
        c = chars[0] if scalar else count
        code = bytes.fromhex('31 00 8B 21 97 A5 E5 F1 01') + bytes((c, 0x12)) + bytes.fromhex('11 78 56 21') + word(input_at) + call('font4Scalar' if scalar else 'font4Batch')
        code += bytes.fromhex('22 C0 83 60 69 22 C2 83 EB 22 C4 83 F5 E1 22 C6 83 21 00 00 39 22 C8 83')
        expected = bytearray(m[0x9000:0xc000])
        if valid:
            for i, char in enumerate(chars):
                px = x + i * 4
                at = (px // 8) * 256 + y - 7
                for row, v in enumerate(narrow[char * 8:char * 8 + 8]):
                    shift = 4 if px % 8 == 0 else 0
                    mask = 15 << shift
                    expected[at + row] = (expected[at + row] & (255 ^ mask)) | (v << shift)
        data = execute(m, code)
        assert data[0x9000:0xc000] == expected, ('pixels', m[s['font4Format']], scalar, chars, x, y)
        assert data[0x83c0:0x83c6] == word(input_at) + bytes((c, 0x12, 0x78, 0x56)), 'registers'
        assert data[0x83c7] == 0xa5 and bool(data[0x83c6] & 1) == (not valid), 'AF/CF'
        assert data[0x83c8:0x83ca] == word(0x8b00), 'SP restored'
        assert data[s['bios_vars.cursorY']:s['bios_vars.cursorY'] + 2] == bytes((y, (x + (count * 4 if valid else 0)) // 2)), 'cursor'
        assert data[0x2800:0x3800] == m[0x2800:0x3800], 'font unchanged'
        assert data[0x4000:0x6000] == m[0x4000:0x6000], 'full phases unchanged'
        checks += 1

    for fmt in range(3):
        prepared = execute(bytearray(base), bytes.fromhex('31 00 8B') + bytes((0x3e, fmt)) + call('font4Prepare'))
        expected = bytes(narrow[c * 8 + y] * (17 if fmt == 2 else 1) for y in range(8) for c in range(256)) if fmt != 1 else bytes((narrow[c * 8 + y] << 4) | narrow[(c + 1) * 8 + y] for c in range(0, 256, 2) for y in range(8))
        assert prepared[0x3000:0x3000 + len(expected)] == expected, ('prepare', fmt)
        assert prepared[0x2800:0x3000] == font
        if fmt != 1:
            assert prepared[0x4000:0x5000] == bytes(value for v in font for value in (v >> 2, (v << 6) & 192))
        else:
            assert prepared[0x4000:0x6000] == base[0x4000:0x6000]
        checks += 1
        for c in range(256):
            render(prepared, bytes((c,)), (c % 96) * 4, 255, scalar=True)
            render(prepared, bytes((c, (c + 1) & 255, (c + 2) & 255, c ^ 1, c, c)), 4 if c & 1 else 0, 7)
        for chars, x, y in [(bytes(range(48)), 0, 255), (bytes(range(47)), 4, 7), (b'A', 4, 255), (b'A' * 48, 192, 7), (b'A' * 47, 196, 255), (b'', 2, 0), (b'A' * 49, 0, 7), (b'A', 2, 7), (b'A', 384, 7), (b'A', 0, 6), (b'AB', 380, 7), (b'AB', 376, 7)]:
            render(prepared, chars, x, y)
        for address, count, x in [(0x7c01, 4, 0), (0x7cff, 47, 4), (0x7dff, 48, 0)]:
            render(prepared, bytes(range(count)), x, 255, input_at=address)
        # Проверка самого экранного компаратора: точный эталон и порча байта.
        m = bytearray(prepared)
        corpus = bytes((32 + i % 224) for i in range(1152))
        m[0x8600:0x8a80] = corpus
        m[s['matrixKind']] = 5 + fmt * 2
        for row in range(24):
            for col in range(24):
                a, b = corpus[row * 48 + col * 2:row * 48 + col * 2 + 2]
                for y in range(8):
                    m[0x9000 + col * 256 + row * 8 + y] = narrow[a * 8 + y] * 16 + narrow[b * 8 + y]
        for bad in (False, True):
            m[s['bitmapResult']:s['bitmapResult'] + 2] = word(s['matchText'])
            if bad: m[0x9000 + 23 * 256 + 191] ^= 1
            data = execute(m, bytes.fromhex('31 00 8B') + call('checkBankFrame'))
            assert data[s['bitmapResult']:s['bitmapResult'] + 2] == word(s['failText'] if bad else s['matchText']), ('checker', fmt, bad)
            checks += 1
    # Новый независимый компаратор 6x8 проверяется на точном экране и порче.
    m = bytearray(base)
    corpus = bytes(32 + i % 224 for i in range(1152))
    m[0x8600:0x8a80] = corpus
    m[s['matrixKind']] = 0
    for row in range(24):
        for group in range(12):
            codes = corpus[row * 48 + group * 4:row * 48 + group * 4 + 4]
            for y in range(8):
                a, b, c, d = (font[code * 8 + y] for code in codes)
                for col, value in enumerate(((a << 2) | (b >> 4), ((b << 4) & 240) | (c >> 2), ((c << 6) & 192) | d)):
                    m[0x9000 + (group * 3 + col) * 256 + row * 8 + y] = value
    for bad in (False, True):
        m[s['bitmapResult']:s['bitmapResult'] + 2] = word(s['matchText'])
        if bad: m[0x9000 + 35 * 256 + 191] ^= 1
        data = execute(m, bytes.fromhex('31 00 8B') + call('checkBankFrame'))
        assert data[s['bitmapResult']:s['bitmapResult'] + 2] == word(s['failText'] if bad else s['matchText']), ('checker6', bad)
        checks += 1
    # Инфострока: геометрия и явный пробел перед числом даже без паддинга.
    for kind, index, geometry in [(0, 0, b'6x8'), (4, 50, b'8x8'), (6, 55, b'4x8')]:
        m = bytearray(base)
        m[s['matrixKind']] = kind
        m[s['matrixBank']] = 0
        m[s['matrixStage']] = 11
        m[s['matrixCase']:s['matrixCase'] + 2] = word(s['matrixCases'] + index * 14)
        m[s['reportMode']] = 0
        m[s['bios_setCursorPos']] = 0xc9
        m[s['bios_printChar']:s['bios_printChar'] + 3] = b'\xC3' + word(0x8300)
        m[s['bios_printString']:s['bios_printString'] + 3] = b'\xC3' + word(0x8320)
        char_stub = bytes.fromhex('F5 E5 2A E0 83 71 23 22 E0 83 E1 F1 C9')
        m[0x8300:0x8300 + len(char_stub)] = char_stub
        string_stub = bytes.fromhex('7E B7 C8 4F CD') + word(s['bios_printChar']) + bytes.fromhex('23 C3 20 83')
        m[0x8320:0x8320 + len(string_stub)] = string_stub
        m[s['print_time_ms']:s['print_time_ms'] + 3] = b'\xC3' + word(0x8350)
        m[0x8350:0x8356] = bytes.fromhex('21 60 83 C3') + word(s['bios_printString'])
        m[0x8360:0x8367] = b'65.536\0'
        m[0x83e0:0x83e2] = word(0x8400)
        data = execute(m, bytes.fromhex('31 00 8B') + call('matrixProgress'))
        end = int.from_bytes(data[0x83e0:0x83e2], 'little')
        line = data[0x8400:end]
        assert f'12/{s["MATRIX_STAGE_COUNT"]} MAIN '.encode() + geometry + b' ' in line, ('geometry', line)
        assert line.endswith(b' 65.536 ms'), ('time separator', line)
        checks += 1
    # Восстановление фазы 0 перед новым направлением Main/DOS.
    restored = execute(bytearray(base), bytes.fromhex('31 00 8B') + call('font4RestorePhase0'))
    assert restored[0x3000:0x3800] == bytes(v << 2 for v in font)
    m = bytearray(base)
    m[s['packedOverlay']] = 1
    m[0x4000:0x5000] = bytes([0xa5]) * 4096
    restored_phase4 = execute(m, bytes.fromhex('31 00 8B') + call('font4RestorePackedOverlay'))
    expected_phase4 = bytes(value for v in font for value in (v >> 2, (v << 6) & 192))
    assert restored_phase4[0x4000:0x5000] == expected_phase4
    assert restored_phase4[s['packedOverlay']] == 0
    checks += 1
    report = dict(checks=checks + 1, formats=3, glyphCodes=256, pixels='PASS', registers='PASS', tails='PASS', packedParity='PASS', checker='PASS', preparation='PASS', phaseRestore='PASS', stack='PASS', inputBoundaries='PASS')
    (args.out / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('FONT4 CPU PASS', checks + 1)

if __name__ == '__main__':
    main()
