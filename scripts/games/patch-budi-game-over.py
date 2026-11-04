"""GAME OVER в пустом рисунке BUDI; код, геометрия и палитра сохранены."""
import argparse
from hashlib import sha256
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / 'assets/games/BUDI'
ARCHIVE = ROOT / 'assets/games/originals/budi-game-over'
TARGETS = {
    'Budi-std.rks': '68b6b503233e6463cfc9558a39ae5d77d26d6ac2eda3c62e5464a8da5adfdb22',
    'Budi2-color-f000.rks': '449588e8175e752819e24a37d1f9184bbb412e3f359b57d14378d7e6685425fc',
    'Budi2-mx.rks': '389c9fa34a21a9795dbb8b863978a48ca467606e00a748ebc68384612cc05eda',
    'Budi-ramfos.cpu': '569192f641e7e9bf612d8accd8cd4998c3554ba352a9947f23af0c64ba8348ff',
}
checksum = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))['checksum']
# Семистрочные жирные буквы со скошенными углами в стиле игрового HUD.
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


def patch(original, name):
    if sha256(original).hexdigest() != TARGETS[name]:
        raise ValueError(f'Неизвестный оригинал: {name}')
    offset = 4 if name.endswith('.rks') else 0
    if offset and checksum(original[4:-2]) != int.from_bytes(original[-2:], 'little'):
        raise ValueError(f'Неверная сумма RKS: {name}')
    start = offset + 0x1400
    if original[start:start + 91] != bytes.fromhex('08 0B 7F') + bytes(88):
        raise ValueError(f'Неожиданный рисунок: {name}')
    fixed = bytearray(original)
    fixed[start + 3:start + 91] = bitmap()
    if offset:
        fixed[-2:] = checksum(fixed[4:-2]).to_bytes(2, 'little')
    return bytes(fixed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    prepared = []
    for name in TARGETS:
        target = GAMES / name
        archive = ARCHIVE / (name + '.bin')
        current = target.read_bytes()
        original = archive.read_bytes() if archive.exists() else current
        fixed = patch(original, name)
        if current not in (original, fixed):
            raise ValueError(f'Рабочая копия изменена вне патча: {name}')
        if args.check and (current != fixed or not archive.exists()):
            raise ValueError(f'Патч или оригинал отсутствует: {name}')
        prepared.append((target, archive, original, fixed))
    if not args.check:
        ARCHIVE.mkdir(parents=True, exist_ok=True)
        for target, archive, original, fixed in prepared:
            if not archive.exists():
                archive.write_bytes(original)
            target.write_bytes(fixed)
    print('OK: GAME OVER, 4 варианта; код, размеры, палитра и оригиналы сохранены')


if __name__ == '__main__':
    main()
