"""Восстановление Almaz2R, Almaz2D и исторического исправленного Almaz2X.

Запуск из любого каталога: python scripts/games/repair-almaz2x.py
Проверка готового файла: python scripts/games/repair-almaz2x.py --check
Оригинал сохранён в assets/games/originals/Almaz2X.bin.
Старый MXOS-вариант: originals/Almaz2X-mxos-legacy.bin.
Новый assets/games/Almaz2X.exe собирается из source-apps/almaz2 отдельно.
"""

import argparse
from hashlib import sha256
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / "assets/games"
ORIGINAL_HASH = "e2e0193a23656a4063d7966b912090bd553edf74f1bd2405ec9ef7a8cd48472c"
MONO_HASH = "4099d26e24974f7aeb518bcbe0040142e71bd29ba38b66678e569d70bba7debc"
FIXED_HASH = "df73430dd8f6d17a40aab8fedfa75ba21663da13ef0c2f888dbccb5d4b0a6713"
RAMFOS_HASH = "3ff4b12a3a213edfd11a076a375231bcb56ebe51b630aec4a06ec22a9c0eee52"
DARK_HASH = "4e72547939a9cc4b8da2bf472d7981519305fed793517133bfaa6d0e9b248ae8"
USER_HASH = "42e2327636bfd0a5a3c4bab3b3cd27fdb61d9643c777240e7bc81f1533c0a3a3"

# Цвет рисунка в старшей тетраде, фон в младшей. Последние четыре
# элемента повторяют 0..3, как в исторической таблице.
USER_PALETTE = bytes((
    0x00,  # 0: пустота — чёрный.
    0x10,  # 1: выход — синий на чёрном.
    0x48,  # 2: кирпичи — красный на тёмно-сером.
    0x70,  # 3: неразрушаемая стена — светло-серый на чёрном.
    0x40,  # 4: камни — тёмно-красный на чёрном.
    0xB0,  # 5: алмазы — светло-голубой на чёрном.
    0xE4,  # 6: квадратики — жёлтый на красном.
    0xF0,  # 7: бабочки — белый на чёрном.
    0x50,  # 8: грибы — фиолетовый на чёрном.
    0xE0,  # 9: игрок — жёлтый на чёрном.
    0x20,  # A: земля/трава — тёмно-зелёный на чёрном.
    0x80,  # B: пыль/дым — тёмно-серый на чёрном.
    0x00, 0x10, 0x48, 0x70,
))
TEXT_STRING_CALLS = (0x0145, 0x019E)
TEXT_CHAR_CALLS = (0x014E, 0x0157, 0x0160, 0x01A7)
TEXT_COLOR = USER_PALETTE[3]  # Цвет текста совпадает с рамкой при общих байтах экрана.
HUD_Y = 11  # Нижняя координата знаков TIME и SCREEN в пикселях BIOS.

# Адрес загрузки равен нулю, поэтому адреса совпадают со смещениями файла.
# Начальный адрес операнда и его размер; команды остаются прежними.
ABI_OPERANDS = (
    (0x003F, 1),  # Возврат: F800h -> bios_reboot (C800h).
    (0x0134, 2),  # Частота звука: C64Eh -> 8FF1h.
    (0x0137, 2),  # Длительность звука: C64Fh -> 8FF2h.
    (0x0140, 2),  # Координаты курсора: C63Eh -> 8FFCh.
    (0x099F, 2),  # Частота звука в проигрывателе мелодий.
    (0x09A4, 2),  # Длительность звука в проигрывателе мелодий.
    (0x09B6, 2),  # Звук: C851h -> bios_beep_Old (C170h).
)


def checked_read(path, expected_hash):
    data = path.read_bytes()
    actual = sha256(data).hexdigest()
    if actual != expected_hash:
        raise ValueError(f"SHA-256 mismatch: {path}: {actual}")
    return data


def repair(original, mono, *, mxos=True):
    result = bytearray(original)
    if mxos:
        for offset, size in ABI_OPERANDS:
            result[offset:offset + size] = mono[offset:offset + size]

    # Исправление надписи SQREEN -> SCREEN в обоих экземплярах строки.
    for offset in (0x0B92, 0x0BB6):
        result[offset] = mono[offset]

    # 16 карт по 512 байт: 16 строк с шагом 32, видимая ширина 24 клетки.
    # Восстанавливаются 27 отличающихся клеток, включая рамки. Авторская
    # структура уровней принимается по Almaz-2.exe. Невидимые столбцы 24..31,
    # сохранённые рабочие переменные и вся цветная подпрограмма сохраняются.
    for offset in range(0x1400, 0x3400):
        if offset % 32 < 24:
            result[offset] = mono[offset]

    changes = [i for i, pair in enumerate(zip(original, result)) if pair[0] != pair[1]]
    if len(result) != 13348 or len(changes) != (42 if mxos else 29):
        raise ValueError("Unexpected patch size")
    if result[0x0886:0x0889] != original[0x0886:0x0889]:
        raise ValueError("Color hook changed")
    if result[0x3400:] != original[0x3400:]:
        raise ValueError("Color routine changed")
    if sha256(result).hexdigest() != (FIXED_HASH if mxos else RAMFOS_HASH):
        raise ValueError("Unexpected output SHA-256")
    return bytes(result)


def dark_background(fixed):
    result = bytearray(fixed)
    # Меняется только фон 9 -> 1; яркость рисунка и остальные фоны прежние.
    # Это отдельный эксперимент, а не восстановление исторической палитры.
    for offset in range(0x3414, 0x3424):
        if result[offset] & 15 == 9:
            result[offset] &= 0xF7
    if sum(a != b for a, b in zip(fixed, result)) != 9:
        raise ValueError("Unexpected dark palette patch size")
    if sha256(result).hexdigest() != DARK_HASH:
        raise ValueError("Unexpected dark palette SHA-256")
    return bytes(result)


def user_colors(fixed):
    result = bytearray(fixed)
    result[0x3414:0x3424] = USER_PALETTE
    if result[0x013C:0x013F] != bytes.fromhex("21 0a 50"):
        raise ValueError("Unexpected TIME coordinates")
    result[0x013D] = HUD_Y
    if result[0x019B:0x019E] != bytes.fromhex("21 89 0b"):
        raise ValueError("Unexpected SCREEN string setup")
    result[0x019B:0x019E] = bytes.fromhex("cd 38 34")
    for calls, bios, helper in ((TEXT_STRING_CALLS, 0xC818, 0x3424),
                                (TEXT_CHAR_CALLS, 0xC809, 0x342E)):
        for offset in calls:
            if result[offset:offset + 3] != b"\xCD" + bios.to_bytes(2, "little"):
                raise ValueError(f"Unexpected text call at {offset:04X}")
            result[offset + 1:offset + 3] = helper.to_bytes(2, "little")
        # Переходник вывода строки/символа. Вход/выход и разрушаемые
        # регистры/флаги — по BIOS; собственные AF сохраняются. PUSH AF;
        # LD A,070h; LD (0FFF8h),A; POP AF; JP bios_printString/Char.
        # По 10 байт и 51 добавленный такт 8080, одна запись регистра цвета.
        result.extend(bytes((0xF5, 0x3E, TEXT_COLOR))
                      + bytes.fromhex("32 f8 ff f1 c3") + bios.to_bytes(2, "little"))
    # 3438h: вместо LD HL,0B89h установить X=0, Y=11 и пропустить
    # начальный HOME (0Ch), который вернул бы Y=8. Шесть кодов сдвига
    # вправо в строке сохраняют прежнюю горизонтальную позицию SCREEN.
    # Вход: нет; выход: HL=0B8Ah; AF/BC/DE и флаги сохраняются.
    # 10 байт, 53 добавленных такта 8080, две записи переменных курсора.
    result.extend(b"\x21" + HUD_Y.to_bytes(2, "little")
                  + bytes.fromhex("22 fc 8f 21 8a 0b c9"))
    if len(result) != 13378 or sha256(result).hexdigest() != USER_HASH:
        raise ValueError("Unexpected user palette output")
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="только проверить готовый файл")
    args = parser.parse_args()
    original = checked_read(GAMES / "originals/Almaz2X.bin", ORIGINAL_HASH)
    mono = checked_read(GAMES / "Almaz-2.exe", MONO_HASH)
    variants = (
        ("Almaz2R.exe", repair(original, mono, mxos=False), RAMFOS_HASH, 29),
        ("originals/Almaz2X-mxos-legacy.bin", repair(original, mono), FIXED_HASH, 42),
        ("Almaz2D.exe", user_colors(repair(original, mono)), USER_HASH, 104),
    )
    # Проверить все назначения до записи первого файла.
    for name, fixed, digest, count in variants:
        target = GAMES / name
        if args.check:
            if target.read_bytes() != fixed:
                raise ValueError(f"Patched file differs: {target}")
        elif target.exists() and sha256(target.read_bytes()).hexdigest() not in (
            ORIGINAL_HASH, digest, DARK_HASH if name == "Almaz2D.exe" else digest
        ):
            raise ValueError(f"Refusing to overwrite unknown file: {target}")
    for name, fixed, digest, count in variants:
        target = GAMES / name
        if not args.check:
            target.write_bytes(fixed)
        print(f"OK: {target}, {len(fixed)} bytes, {count} patched/added bytes, SHA-256 {digest}")


if __name__ == "__main__":
    main()
