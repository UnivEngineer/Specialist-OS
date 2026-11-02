"""CPU-проверка разбора LAUNCH, KOI-7 и возврата; файловые вызовы заменены.

Маппер и ВВ55 не моделируются. Аппаратные режимы проверяются через Emu80.
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
    out = ROOT / 'docs/compat/launch-001'
    out.mkdir(parents=True, exist_ok=True)
    ds, ls = symbols('DOS'), symbols('LAUNCH')
    results = {}
    for mode, ext in ((mode, ext) for mode in ('MX', 'STD', 'MON', 'MON:Monitor2')
                      for ext in ('EXE', 'RKS')):
        m = bytearray(65536)
        for base, path in ((0xC000, 'bin/DOS.sys'), (0xE000, 'assets/programs/Monitor2.mon'),
                           (0xF800, 'bin/LAUNCH.com')):
            data = (ROOT / path).read_bytes()
            m[base:base+len(data)] = data
        m[0x8FDF:0x9000] = m[ds['initVars']:ds['initVars']+33]
        m[0x8FEA] = 0
        m[0xFFF8] = 0xF0
        m[0xF7FA] = 0x5A
        text = f'-{mode} A:GAME.{ext}\0'.encode('ascii')
        m[0x8400:0x8400+len(text)] = text
        m[0x8200:0x820C] = bytes.fromhex('31 df 8f 11 00 84 cd 00 f8 c3 00 83')
        m[0] = 0xC9
        for entry in (0xC848, 0xC851, 0xC866):
            m[entry:entry+2] = bytes.fromhex('b7 c9')
        m[0xC818] = 0xC9
        m[0xC85A:0xC85D] = bytes.fromhex('c3 00 86')
        # Второй запрос имени — монитор: его размер равен 4096 (N-1=0FFFh).
        # Первый запрос сохраняет фактический указатель имени программы.
        # Второй запрос имени — монитор, как и в прежней проверке.
        stub = bytes.fromhex('3a 00 85 3c 32 00 85 fe 02 ca 11 86 22 02 85 b7 c9 21 ff 0f 22')
        stub += (ls['descriptor'] + 28).to_bytes(2, 'little') + b'\xC9'
        m[0x8600:0x8600+len(stub)] = stub
        tag = mode.replace(':', '-') + '-' + ext
        source, target = out / f'cpu-{tag}-input.bin', out / f'cpu-{tag}-out.bin'
        source.write_bytes(m)

        def run(end):
            r = subprocess.run([str(args.ticks.resolve()), '-m8080', '-pc', '8200',
                    '-end', f'{end:04x}', '-counter', '10000000', '-output', str(target),
                    str(source)], capture_output=True, text=True, check=True, timeout=20)
            b = target.read_bytes()
            assert int.from_bytes(b[65542:65544], 'little') == end, r.stdout
            return b

        b = run(0)
        name_ptr = int.from_bytes(b[0x8502:0x8504], 'little')
        expected_name = f'A:GAME.{ext}\0'.encode('ascii')
        assert b[name_ptr:name_ptr+len(expected_name)] == expected_name
        if mode == 'MX':
            assert b[0x8FEA] == 0
            assert b[0xF7FA] == 0x5A and b[0xFFF8] == 0xF0
        elif mode == 'STD':
            assert b[0x8FEA] == 0xFF
            assert b[0xF7FA] == 3 and b[0xFFF8] == 0xF0
        else:
            # У Monitor-2 8FEAh — другое поле, не флаг KOI-7 MXOS.
            # Его исходный символьный движок сам использует KOI-7.
            assert b[0xC000:0xC010] == m[0xE000:0xE010]
            assert b[0xF7FA] == 3 and b[0xFFF8] == 0xF0
        results[tag] = {'entry_sp': b[65544:65546][::-1].hex(), '8FEA': b[0x8FEA],
                        'filename': expected_name[:-1].decode('ascii')}
        if not mode.startswith('MON'):
            b = run(0x8300)
            assert b[0x8FEA] == 0 and b[65544:65546] == bytes.fromhex('df 8f')
            results[tag]['return_and_encoding_restored'] = True
    (out / 'cpu-formats.json').write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
