"""CPU-проверка ключа -RF: строка RAMRUN, аргументы и отказ неверных ключей.

Выполнение останавливается перед bios_fileExec; подмена ядра здесь не моделируется.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ticks', type=Path)
    args = parser.parse_args()
    out = ROOT / 'docs/compat/launch-rf-001'
    out.mkdir(parents=True, exist_ok=True)
    symbols = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
               (ROOT / 'bin/DOS.sym').read_text(encoding='utf-8-sig'), re.M)}
    cases = [('-RF H:GAME.RKS /X one', 'H:GAME.RKS /X one'),
             ('-rf A:GAME.EXE', 'A:GAME.EXE'),
             ('  -rF   B:GAME.RKS  a b', 'B:GAME.RKS  a b'),
             ('-RF', None), ('-RF ', None), ('-RFX GAME.RKS', None),
             ('-RF:OTHER GAME.EXE', None)]
    results = []
    for number, (text, tail) in enumerate(cases):
        memory = bytearray(65536)
        for base, file in ((0xC000, 'DOS.sys'), (0xF800, 'LAUNCH.com')):
            data = (ROOT / 'bin' / file).read_bytes()
            memory[base:base+len(data)] = data
        memory[0x8FDF:0x9000] = memory[symbols['initVars']:symbols['initVars']+33]
        memory[0xC818] = 0xC9
        memory[0x8400:0x8400+len(text)+1] = text.encode('ascii') + b'\0'
        memory[0x8200:0x820C] = bytes.fromhex('31 00 dc 11 00 84 cd 00 f8 c3 00 83')
        source, target = out / f'rf-{number}-in.bin', out / f'rf-{number}-out.bin'
        source.write_bytes(memory)
        stop = 0xC85D if tail is not None else 0x8300
        result = subprocess.run([str(args.ticks.resolve()), '-m8080', '-pc', '8200',
            '-end', f'{stop:04x}', '-counter', '1000000', '-output', str(target), str(source)],
            capture_output=True, text=True, check=True, timeout=20)
        data = target.read_bytes()
        assert int.from_bytes(data[65542:65544], 'little') == stop, (text, result.stdout)
        if tail is not None:
            expected = ('A:RAMRUN.COM ' + tail).encode('ascii') + b'\0'
            assert data[0xE800:0xE800+len(expected)] == expected, text
            assert data[0xF800:0xF810] == memory[0xF800:0xF810]
        results.append({'arguments': text, 'dispatch': tail is not None})
    (out / 'cpu.json').write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
    print(f'OK: {len(cases)} случаев -RF; имя программы и аргументы сохранены')


if __name__ == '__main__':
    main()
