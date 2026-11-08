"""8080: восемь строк без масок, границы, CR/LF и десятичный номер этапа."""
from pathlib import Path
import argparse
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/font-matrix-004/cpu')
    parser.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    s = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT / 'bin/FONTCOM.sym').read_text(), re.M)}
    base = bytearray((i * 37 + (i >> 8) * 13) & 255 for i in range(65536))
    program = (ROOT / 'bin/FONTCOM.i80').read_bytes()
    base[:len(program)] = program
    font = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
    word = lambda x: x.to_bytes(2, 'little')
    base[s['bios_vars.charGen']:s['bios_vars.charGen'] + 2] = word(0x2800 // 8)
    # Ядро полностью развёрнуто: восемь чтений шрифта и восемь записей экрана.
    kernel = base[s['aligned8Pixels']:s['aligned8Continue']]
    assert kernel == bytes.fromhex('1A 77 1C 2C') * 7 + bytes.fromhex('1A 77')

    def execute(m, code, pc=0x8000):
        code += b'\xc3\x80\x8b'
        m[pc:pc + len(code)] = code
        (args.out / 'input.bin').write_bytes(m)
        subprocess.run([str(args.ticks), '-m8080', '-pc', f'{pc:04x}', '-end', '8b80', '-counter', '20000000', '-output', str(args.out / 'output.bin'), str(args.out / 'input.bin')], capture_output=True, check=True, timeout=20)
        return (args.out / 'output.bin').read_bytes()

    checks = 0
    def render(chars, x, y, scalar=False):
        nonlocal checks
        m = bytearray(base)
        m[0x7800:0x7800 + len(chars)] = chars
        m[s['bios_vars.cursorY']:s['bios_vars.cursorY'] + 2] = bytes((y, x // 2))
        count = len(chars)
        valid = (not scalar and count == 0) or (count <= 48 and x % 8 == 0 and x + count * 8 <= 384 and y >= 7)
        c = chars[0] if scalar else count
        code = bytearray(bytes.fromhex('31 00 8B 21 97 A5 E5 F1 01') + bytes((c, 0x12)) + bytes.fromhex('11 78 56 21 00 78 CD') + word(s['aligned8Scalar' if scalar else 'aligned8Batch']))
        code += b'\x22\xc0\x83' + bytes.fromhex('60 69 22 C2 83 EB 22 C4 83 F5 E1 22 C6 83')
        expected = bytearray(m[0x9000:0xc000])
        if valid:
            for col, char in enumerate(chars):
                at = (x // 8 + col) * 256 + y - 7
                expected[at:at + 8] = font[char * 8:char * 8 + 8]
        data = execute(m, code)
        assert data[0x9000:0xc000] == expected, ('pixels', scalar, chars, x, y)
        assert data[0x83c0:0x83c6] == word(0x7800) + bytes((c, 0x12, 0x78, 0x56)), 'registers'
        assert data[0x83c7] == 0xa5 and bool(data[0x83c6] & 1) == (not valid), 'AF/CF'
        assert data[s['bios_vars.cursorY']:s['bios_vars.cursorY'] + 2] == bytes((y, (x + (count * 8 if valid else 0)) // 2))
        assert data[0x2800:0x3800] == base[0x2800:0x3800]
        assert data[0xdb90:0xeb90] == base[0xdb90:0xeb90], 'no cache'
        checks += 1

    for c in range(256):
        render(bytes((c,)), (c % 48) * 8, 255, scalar=True)
        render(bytes((c,)) * 48, 0, 7)
    for chars, x, y in [(bytes(range(48)), 0, 255), (b'', 2, 0), (b'A' * 49, 0, 7), (b'A', 2, 7), (b'A', 384, 7), (b'A', 0, 6), (b'AB', 376, 7), (b'AB', 368, 7)]:
        render(chars, x, y)

    # Переводы строк и управляющие байты, включая CRLF между блоками 256 байт.
    for source in [b'A\r\nB', b'A\rB\nC', b'\r\n\r\n', b'\r\r\n', b'A' * 255 + b'\r\nB', bytes(range(256)), b'\xc0\xff\r\n\xa3']:
        m = bytearray(base)
        m[s['corpusLastCR']] = 0
        code = bytearray(bytes.fromhex('31 00 8B'))
        for i, value in enumerate(source):
            code += bytes((0x3e, value, 0xcd)) + word(s['corpusNormalize']) + bytes.fromhex('F5 E1 22') + word(0x8000 + i * 2)
        data = execute(m, code, pc=0x6000)
        got = bytes(data[0x8001 + i * 2] for i in range(len(source)) if not data[0x8000 + i * 2] & 1)
        expected = bytes(32 if c < 32 else c for c in source.replace(b'\r\n', b'\n').replace(b'\r', b'\n'))
        assert got == expected, ('CRLF', source, got, expected)
        checks += 1

    # Заглушка экранного ABI записывает C, сохраняя используемые регистры.
    for stage in (0, 1, 9, 10, 12, 99, 100, 108, 255):
        m = bytearray(base)
        m[s['bios_printChar']:s['bios_printChar'] + 3] = b'\xc3\x50\x83'
        m[0x8350:0x8350 + 14] = bytes.fromhex('F5 E5 2A E0 83 71 23 22 E0 83 E1 F1 C9 00')
        m[0x83e0:0x83e2] = word(0x8400)
        code = bytearray(bytes.fromhex('31 00 8B') + bytes((0x3e, stage, 0xcd)) + word(s['matrixProgressNumber']))
        data = execute(m, code)
        end = int.from_bytes(data[0x83e0:0x83e2], 'little')
        assert data[0x8400:end] == str(stage).encode(), ('stage', stage)
        checks += 1
    report = dict(checks=checks, glyphCodes=256, unrolledRows=8, screenReads=0, screenWrites=8, cacheBytes=0, registers='PASS', pixels='PASS', CRLF='PASS', stageCounter='PASS')
    (args.out / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('ALIGNED8 CPU PASS', checks)


if __name__ == '__main__':
    main()
