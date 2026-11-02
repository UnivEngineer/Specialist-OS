"""Копии «Бурана» в платформенных контейнерах из побайтных legacy-эталонов."""
import argparse
import hashlib
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / 'assets/games/Buran'
SOURCES = {
    'Buran-std.rks': '092f321c30b6d89d7df4136a28ca9512d0285782e992ad7a841c4d52399fe1fa',
    'Buran-starwars-std.rks': '8706c2ae313ccdc1ea23368524d1a02b9213a727f906f484fb00b1671790bd68',
    'Buran-starwars-std.exe': 'cbcc725cb0916a3b88600650f4b2a2398353e9e074429d0fdbfeccc029180380',
    'Buran-starwars-std-tape.rks.bin': '30243a274207e3ce88d5bfb3cbbf5a4c6db564c549a0256246afaa6abd68a6e8',
    'Buran-starwars-std-germanovitsh.rks': '31b15797983393e9c3b9c5e7ea80eeb54c35a249a99fb457304ee40e3803853a',
    'Buran-ramfos.exe': '471e61f6702042ffc007b7b408297108c546a6198a679c4bd185361654260e66',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    helpers = runpy.run_path(str(Path(__file__).with_name('repair-rks-checksums.py')))
    parse, checksum = helpers['parse'], helpers['checksum']
    original = {}
    for name, expected in SOURCES.items():
        data = (GAMES / 'legacy' / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError(f'Неизвестный оригинал: {name}')
        original[name] = data
    # Старый EXE фактически содержит нагрузку STD и её хвост суммы RKS.
    if original['Buran-starwars-std.exe'] != original['Buran-starwars-std.rks'][4:]:
        raise ValueError('Исторические EXE/RKS не соответствуют друг другу')
    prepared = {}
    for name in ('Buran-std.rks', 'Buran-starwars-std.rks'):
        source = original[name]
        start, end, data, tail = parse(source)
        if start != 0 or len(tail) != 2:
            raise ValueError(f'Неизвестная структура RKS: {name}')
        prepared[name] = source[:4] + data + checksum(data).to_bytes(2, 'little')
    prepared['Buran-ramfos.exe'] = original['Buran-ramfos.exe']
    for name, data in prepared.items():
        target = GAMES / name
        if args.check:
            if not target.exists() or target.read_bytes() != data:
                raise ValueError(f'Платформенная копия отличается: {name}')
        else:
            target.write_bytes(data)
    print('Проверены 6 исторических файлов и 3 платформенные копии «Бурана».')


if __name__ == '__main__':
    main()
