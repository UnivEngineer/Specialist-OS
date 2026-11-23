"""Собирает четыре доработанных EXE BUDI из трёх архивных оригиналов."""
import argparse
from dataclasses import replace
from hashlib import sha256
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / 'assets/games/families/budi'
ORIGINALS = {
    'Budi-std.rks': '68b6b503233e6463cfc9558a39ae5d77d26d6ac2eda3c62e5464a8da5adfdb22',
    'Budi2-std.rks': '449588e8175e752819e24a37d1f9184bbb412e3f359b57d14378d7e6685425fc',
    'Budi-ramfos.cpu': '569192f641e7e9bf612d8accd8cd4998c3554ba352a9947f23af0c64ba8348ff',
}
VARIANTS = (
    ('Budi-std.exe', 'Budi-std.rks', 3),
    ('Budi-ramfos.exe', 'Budi-ramfos.cpu', 4),
    ('Budi2-std.exe', 'Budi2-std.rks', 3),
    ('Budi2-std-fff8.exe', 'Budi2-std.rks', 3),
)
COLOR_WRITES = (0x0005, 0x000D, 0x0282, 0x0726, 0x080E)
container = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))

FONT = {
    'G': (0x1E, 0x33, 0x60, 0x6F, 0x63, 0x33, 0x1E),
    'A': (0x1C, 0x36, 0x63, 0x63, 0x7F, 0x63, 0x63),
    'M': (0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63),
    'E': (0x7F, 0x60, 0x60, 0x7E, 0x60, 0x60, 0x7F),
    'O': (0x1E, 0x33, 0x63, 0x63, 0x63, 0x33, 0x1E),
    'V': (0x63, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x08),
    'R': (0x7E, 0x63, 0x63, 0x7E, 0x6C, 0x66, 0x63),
    ' ': (0,) * 7,
}


def bitmap():
    # Формат игры: высота 8, ширина 11 байт, столбцы по 8 строк.
    result = bytearray(88)
    for column, char in enumerate(' GAME OVER '):
        for row, bits in enumerate(FONT[char]):
            result[column * 8 + row] = bits << 1
    return bytes(result)


def prepare():
    originals = {}
    for name, expected in ORIGINALS.items():
        path = GAMES / 'originals' / name
        data = path.read_bytes()
        if sha256(data).hexdigest() != expected:
            raise ValueError(f'Неизвестный исторический файл: {name}')
        originals[name] = container['decode'](data, path.suffix[1:])

    prepared = {}
    for name, source, mode in VARIANTS:
        image = originals[source]
        payload = bytearray(image.data)
        if name == 'Budi2-std-fff8.exe':
            for at in COLOR_WRITES:
                if payload[at:at + 3] != bytes.fromhex('32 00 F0'):
                    raise ValueError(f'Неожиданный код BUDI2 по адресу {at:04X}h')
                payload[at + 1:at + 3] = bytes.fromhex('F8 FF')
        if payload[0x1400:0x145B] != bytes.fromhex('08 0B 7F') + bytes(88):
            raise ValueError(f'Неожиданный рисунок: {source}')
        payload[0x1403:0x145B] = bitmap()
        prepared[name] = container['encode'](replace(image, data=bytes(payload), mode=mode), 'exe')
    return prepared


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for name, data in prepare().items():
        target = GAMES / 'patched' / name
        if args.check:
            if target.read_bytes() != data:
                raise ValueError(f'Рабочая копия отличается: {name}')
        else:
            target.write_bytes(data)
    print('OK: 3 исторических файла, 4 EXE с GAME OVER')


if __name__ == '__main__':
    main()
