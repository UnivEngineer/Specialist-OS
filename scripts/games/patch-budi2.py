"""BUDI2: перенести пять записей цвета с F000h на регистр MX FFF8h.

Коды цвета сохранены без преобразования. Исходный RKS архивируется побайтно,
сумма контейнера пересчитывается штатной хостовой формулой.
"""
import argparse
from hashlib import sha256
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
TARGET = ROOT / 'assets/games/BUDI/Budi2-mx.rks'
ORIGINAL = ROOT / 'assets/games/originals/budi2/BUDI2.rks.bin'
ORIGINAL_SHA = '449588e8175e752819e24a37d1f9184bbb412e3f359b57d14378d7e6685425fc'
WRITES = (0x0005, 0x000D, 0x0282, 0x0726, 0x080E)
checksum = runpy.run_path(str(Path(__file__).with_name('repair-rks-checksums.py')))['checksum']


def patch(original):
    if sha256(original).hexdigest() != ORIGINAL_SHA:
        raise ValueError('Неизвестный оригинал BUDI2')
    if checksum(original[4:-2]) != int.from_bytes(original[-2:], 'little'):
        raise ValueError('Неверная сумма оригинала BUDI2')
    fixed = bytearray(original)
    for address in WRITES:
        offset = 4 + address  # Четырёхбайтный заголовок RKS, нагрузка с 0000h.
        if fixed[offset:offset + 3] != bytes.fromhex('32 00 F0'):
            raise ValueError(f'Неожиданный код на {address:04X}h')
        fixed[offset + 1:offset + 3] = bytes.fromhex('F8 FF')
    fixed[-2:] = checksum(fixed[4:-2]).to_bytes(2, 'little')
    return bytes(fixed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    current = TARGET.read_bytes()
    original = ORIGINAL.read_bytes() if ORIGINAL.exists() else current
    color_only = patch(original)
    game_over = runpy.run_path(str(Path(__file__).with_name('patch-budi-game-over.py')))
    fixed = game_over['patch'](color_only, TARGET.name)
    if current not in (original, fixed):
        raise ValueError('BUDI2 изменена вне этого патча')
    if args.check:
        if current != fixed or not ORIGINAL.exists():
            raise ValueError('Патч или сохранённый оригинал отсутствует')
    else:
        ORIGINAL.parent.mkdir(parents=True, exist_ok=True)
        if not ORIGINAL.exists():
            ORIGINAL.write_bytes(original)
        TARGET.write_bytes(fixed)
    print(f'OK: {TARGET}, {len(fixed)} bytes, SHA-256 {sha256(fixed).hexdigest()}')


if __name__ == '__main__':
    main()
