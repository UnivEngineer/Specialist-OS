"""Проверка аргументов FORMAT в CPU-модели без обращения к накопителям.

Исполнение останавливается на входе в форматирование либо подтверждение.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def symbols(name):
    return {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
            (ROOT / f'bin/{name}.sym').read_text(encoding='utf-8-sig'), re.M)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('ticks', type=Path)
    args = p.parse_args()
    out = ROOT / 'docs/format/args-001'
    out.mkdir(parents=True, exist_ok=True)
    s = symbols('FORMAT')
    cases = [
        ('B: -R -Y', 'Format', 'B', True, True),
        ('-Y -R c:', 'Format', 'C', True, True),
        ('  h:   -y  ', 'Format', 'H', False, True),
        ('B: -R -R -Y', 'Format', 'B', True, True),
        ('B:', 'ConfirmRequest', 'B', False, False),
        ('-R B:', 'ConfirmRequest', 'B', True, False),
        ('', 'ConfirmRequest', 'B', False, False),
        ('-R -Y', 'Format', 'B', True, True),
        ('B R Y', None, None, None, None),
        ('B: -Z -Y', None, None, None, None),
        ('B: -RY', None, None, None, None),
        ('B: C: -Y', None, None, None, None),
        ('I: -Y', None, None, None, None),
        ('B: -Y UNKNOWN', None, None, None, None),
    ]
    results = []
    for number, (text, label, drive, ram, yes) in enumerate(cases):
        m = bytearray(65536)
        for base, file in ((0xC000, 'DOS'), (0xF100, 'FORMAT')):
            data = (ROOT / f'bin/{file}.{"sys" if file == "DOS" else "com"}').read_bytes()
            m[base:base+len(data)] = data
        m[0xC818] = 0xC9            # печать не нужна для проверки парсера
        m[0xC803:0xC806] = bytes.fromhex('3e 42 c9')  # интерактивный выбор B
        m[0x8200:0x820C] = bytes.fromhex('31 df 8f 11 00 84 cd 00 f1 c3 00 83')
        arg = text.encode('ascii') + b'\r\0'
        m[0x8400:0x8400+len(arg)] = arg
        source, target = out / f'{number:02}-input.bin', out / f'{number:02}-out.bin'
        source.write_bytes(m)
        stop = s[label] if label else 0x8300
        r = subprocess.run([str(args.ticks.resolve()), '-m8080', '-pc', '8200',
                '-end', f'{stop:04x}', '-counter', '1000000', '-output', str(target),
                str(source)], capture_output=True, text=True, check=True, timeout=10)
        b = target.read_bytes()
        assert int.from_bytes(b[65542:65544], 'little') == stop, (text, r.stdout)
        if label:
            assert b[65539] == ord(drive), text  # старший байт BC — буква диска
            assert bool(b[s['v_FormatRAMdisk']]) == ram, text
            assert bool(b[s['v_FormatConfirmed']]) == yes, text
        results.append({'arguments': text, 'result': label or 'rejected', 'disk_access': False})
    (out / 'checks.json').write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
    print(f'OK: {len(results)} argument cases; no disk access')


if __name__ == '__main__':
    main()
