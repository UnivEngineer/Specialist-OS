"""ADSKOK: ограничить очистку нижней памятью и сохранить самопроверку игры.

Исходный упакованный RKS сохраняется в originals. Сумма упаковщика сохраняется
компенсацией в неисполняемом байте; внешняя сумма RKS пересчитывается.
"""
import argparse
from hashlib import sha256
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TARGET = ROOT / 'assets/games/families/adskok/repaired/Adskok.rks'
ORIGINAL = ROOT / 'assets/games/families/adskok/originals/Adskok.rks.bin'
ORIGINAL_SHA = 'cdabd464f516f97581f9f32dd35599911eed3cef6eb88c91e15bcbc6ed5b062d'
REGRESSION_SHA = '117c6047598743c76ac368c24c0b0311939a3b539b04250b4354f1dedaee3f93'


def patch(original):
    if sha256(original).hexdigest() != ORIGINAL_SHA:
        raise ValueError('Неизвестный оригинал ADSKOK')
    fixed = bytearray(original)
    # Смещения в RKS, включая четыре байта заголовка.
    # В распакованном коде: 73F6h LXI B,0A00h -> LXI B,0300h.
    assert fixed[0x4567:0x456A] == bytes.fromhex('01 00 0A')
    fixed[0x4569] = 0x03
    # 7396h: байт между RET на 7395h и входом на 7397h.
    # +7 компенсирует -7 в счётчике; код восстановления зависит от суммы.
    assert fixed[0x4506:0x4509] == bytes.fromhex('C9 E0 C5')
    fixed[0x4507] = 0xE7
    # Игра проверяет байт D6h операнда CALL 73D6h по адресу 6B8Bh.
    assert fixed[0x3CFB:0x3CFE] == bytes.fromhex('CD D6 73')
    # Внешняя сумма RKS пересчитывается существующей хостовой формулой.
    payload = fixed[4:-2]
    total = sum(payload[:-1]) * 0x101
    checksum = (total & 0xFF00) | (((total & 255) + payload[-1]) & 255)
    fixed[-2:] = checksum.to_bytes(2, 'little')
    return bytes(fixed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    current = TARGET.read_bytes()
    original = ORIGINAL.read_bytes() if ORIGINAL.exists() else current
    fixed = patch(original)
    if current not in (original, fixed) and sha256(current).hexdigest() != REGRESSION_SHA:
        raise ValueError('ADSKOK изменён вне этого патча')
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
