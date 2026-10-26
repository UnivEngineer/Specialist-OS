"""Ограниченные CPU-проверки Almaz2X на внешнем z88dk-ticks -m8080.

python scripts/games/check-almaz2x.py <путь-к-z88dk-ticks.exe>
Требуется bin/DOS.sys. Временные образы создаются в .codex-tmp/almaz2x.
Клавиатура, печать текста и звук заменяются заглушками. Это проверка кода
игры и сохранности DOS, а не эмуляция аппаратуры Специалиста или прохождение.
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

from importlib.util import module_from_spec, spec_from_file_location


ROOT = Path(__file__).resolve().parents[2]
TMP = ROOT / ".codex-tmp/almaz2x"
LIMIT = 100_000_000


def require(condition, message):
    if not condition:
        raise ValueError(message)


def word(value):
    return value.to_bytes(2, "little")


def memory(game):
    data = bytearray(65536)
    data[:len(game)] = game
    dos = (ROOT / "bin/DOS.sys").read_bytes()
    data[0xC000:0xC000 + len(dos)] = dos
    # Печать и звук ничего не меняют; клавиатура возвращает отсутствие клавиши.
    for address, code in (
        (0xC170, "c9"), (0xC818, "c9"), (0xC809, "c9"),
        (0xC81B, "3e ff c9"), (0xC812, "3e ff c9"),
        (0xC851, "c3 00 50"),  # Ловушка ошибочного вызова файловой функции.
    ):
        block = bytes.fromhex(code)
        data[address:address + len(block)] = block
    return data


def run(ticks, name, data, pc=0x4000, end=0x5000):
    source = TMP / f"{name}.bin"
    target = TMP / f"{name}.out.bin"
    source.write_bytes(data)
    result = subprocess.run(
        [str(ticks), "-m8080", "-pc", f"{pc:04x}", "-end", f"{end:04x}",
         "-counter", str(LIMIT), "-output", str(target), str(source)],
        capture_output=True, text=True, timeout=30, check=True,
    )
    cycles = int(result.stdout.strip())
    require(cycles < LIMIT, f"{name}: execution limit reached")
    output = target.read_bytes()
    # Ticks дописывает регистры после 64 КБ RAM; проверяем реальный PC,
    # чтобы исчерпание лимита не принять за достижение точки останова.
    require(len(output) >= 65546, f"{name}: invalid RAM/register dump")
    require(int.from_bytes(output[65542:65544], "little") == end,
            f"{name}: did not reach expected PC")
    return output[:65536], cycles


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ticks", type=Path)
    args = parser.parse_args()
    TMP.mkdir(parents=True, exist_ok=True)
    sys.dont_write_bytecode = True
    spec = spec_from_file_location("repair", Path(__file__).with_name("repair-almaz2x.py"))
    repair = module_from_spec(spec)
    spec.loader.exec_module(repair)
    games = ROOT / "assets/games"
    mono = repair.checked_read(games / "Almaz-2/Almaz2-original.exe", repair.MONO_HASH)
    original = repair.checked_read(games / "Almaz-2/Almaz2-original-ramfos-broken.exe", repair.ORIGINAL_HASH)
    fixed = repair.checked_read(games / "Almaz-2/legacy/Almaz2X-mxos-legacy.bin", repair.FIXED_HASH)
    dark = repair.checked_read(games / "Almaz-2/legacy/Almaz2D.exe", repair.USER_HASH)
    ramfos_game = repair.checked_read(games / "Almaz-2/Almaz2-original-ramfos.exe", repair.RAMFOS_HASH)
    require(repair.repair(original, mono) == fixed, "Patch reproduction failed")
    require(repair.user_colors(fixed) == dark, "User palette reproduction failed")
    allowed = set(range(0x3414, 0x3424))
    allowed.update((0x013D, 0x019B, 0x019C, 0x019D))
    for offset in repair.TEXT_STRING_CALLS + repair.TEXT_CHAR_CALLS:
        allowed.update((offset + 1, offset + 2))
    require(all(a == b or i in allowed for i, (a, b) in enumerate(zip(fixed, dark))),
            "User variant changed game logic or maps")
    require(dark[0x3414:0x3424] == repair.USER_PALETTE, "Unexpected user palette")
    require(repair.repair(original, mono, mxos=False) == ramfos_game, "Ramfos patch reproduction failed")
    report = {"patched_bytes": 42, "intro_cycles": {}, "levels": [], "tiles": []}

    # Обе программы ядра запускаются с 0000h. Сначала исполняем их
    # загрузчики до входа в оболочку; проверяем именно результат переноса.
    ramfos_images = {}
    for filename, source, destination, size, shell in (
        ("Ramfos.EXE", 0x0030, 0xC000, 0x12A0, 0x12D0),
        ("RAMFOS.COM", 0x0430, 0xC600, 0x0BF0, 0x1020),
    ):
        program = (ROOT / "assets/programs" / filename).read_bytes()
        data = bytearray(65536)
        data[:len(program)] = program
        loaded, cycles = run(args.ticks, f"loader-{filename}", data, 0, 0xF800)
        require(loaded[destination:destination + size] == program[source:source + size],
                f"{filename}: kernel relocation failed")
        require(loaded[0xF800:0xFFE0] == program[shell:shell + 0x07E0],
                f"{filename}: shell relocation failed")
        ramfos_images[filename] = loaded
    report["kernel_loaders_from_0000"] = list(ramfos_images)

    # Вариант для Ramfos проверяем с настоящим перемещённым ядром:
    # звуковую процедуру C851h оставляем, ввод и печать заменяем заглушками.
    data = bytearray(ramfos_images["Ramfos.EXE"])
    data[:len(ramfos_game)] = ramfos_game
    for address, code in ((0xC818, "c9"), (0xC809, "c9"), (0xC81B, "3e ff c9")):
        block = bytes.fromhex(code)
        data[address:address + len(block)] = block
    output, cycles = run(args.ticks, "ramfos-intro", data, 9, 0x0038)
    require(output[0xC800:0xD2A0] == data[0xC800:0xD2A0], "Ramfos code overwritten")
    require(output[0xFFF8] == ramfos_game[0x3414 + 3], "Ramfos color output failed")
    report["intro_cycles"]["ramfos_real_sound"] = cycles

    for name, game in (("mono", mono), ("original", original), ("fixed", fixed), ("dark", dark)):
        data = memory(game)
        output, cycles = run(args.ticks, f"{name}-intro", data, 9,
                             0x5000 if name == "original" else 0x0038)
        changed = [i for i in range(0xC000, 0xDB00) if output[i] != data[i]]
        if name == "original":
            require(changed == [0xC64E, 0xC64F], "Original failure not reproduced")
        else:
            require(not changed, f"{name}: DOS overwritten")
        report["intro_cycles"][name] = cycles

    for level in range(16):
        outputs = []
        for name, game in (("mono", mono), ("fixed", fixed), ("dark", dark)):
            data = memory(game)
            # Бинарники содержат разные сохранённые рабочие переменные,
            # включая состояние ГПСЧ в 0B7Ch. Сравниваем одинаковый старт.
            data[0x0B74:0x0B89] = mono[0x0B74:0x0B89]
            # Драйвер: SP=7FFFh; указатели карты/таймера, номер уровня,
            # число алмазов; загрузка карты, отрисовка, один полный игровой
            # кадр (четыре прохода по клеткам). Остановка на очередном вводе.
            code = (bytes.fromhex("31 ff 7f 21") + word(0x1400 + level * 512)
                    + bytes.fromhex("22 7f 0b 21") + word(0x1200 + level * 4)
                    + bytes.fromhex("22 87 0b 3e") + bytes([level])
                    + bytes.fromhex("32 7e 0b 3a") + word(0x0BC0 + level)
                    + bytes.fromhex("32 86 0b cd 36 02 cd 2b 09 c3 40 00"))
            data[0x4000:0x4000 + len(code)] = code
            output, _ = run(args.ticks, f"{name}-level-{level + 1:02}", data, end=0x0038)
            require(output[0xC000:0xDB00] == data[0xC000:0xDB00], "DOS overwritten")
            outputs.append(output)
        require(all(outputs[0][i] == output[i] for output in outputs[1:]
                    for i in range(0x8000, 0x8200) if i % 32 < 24),
                f"Level {level + 1}: game state differs")
        require(all(outputs[0][0x9000:0xC000] == output[0x9000:0xC000] for output in outputs[1:]),
                f"Level {level + 1}: screen bitmap differs")
        report["levels"].append(level + 1)

    for case in range(4 * 16):
        bank, tile = divmod(case, 16)
        outputs = []
        for name, game in (("mono", mono), ("fixed", fixed), ("dark", dark)):
            data = memory(game)
            data[0x0B74:0x0B76] = word(0x0C00 + bank * 0x180)
            # AF=(A0h+tile):D7h, HL=BEEFh, BC=0304h, SP=7FFFh.
            # После вызова 0885h сохраняем AF, HL, SP для проверки стека,
            # исходных флагов и старших битов A через цветную вставку.
            code = (bytes.fromhex("31 ff 7f 21") + word(((0xA0 + tile) << 8) | 0xD7)
                    + bytes.fromhex("e5 f1 21 ef be 01 04 03 cd 85 08 22 02 60")
                    + bytes.fromhex("f5 e1 22 00 60 21 00 00 39 22 04 60 c3 00 50"))
            data[0x4000:0x4000 + len(code)] = code
            output, _ = run(args.ticks, f"{name}-bank-{bank}-tile-{tile:02}", data)
            outputs.append(output)
        require(all(outputs[0][0x6000:0x6006] == output[0x6000:0x6006] for output in outputs[1:]),
                f"Tile {tile}: AF/HL/SP differ")
        require(all(outputs[0][0x9000:0xC000] == output[0x9000:0xC000] for output in outputs[1:]),
                f"Tile {tile}: screen bitmap differs")
        require(outputs[1][0xFFF8] == original[0x3414 + tile], f"Tile {tile}: wrong color")
        require(outputs[2][0xFFF8] == dark[0x3414 + tile], f"Tile {tile}: wrong user color")
        report["tiles"].append({"bank": bank, "tile": tile})

    # Проверить, что оба переходника действительно выбирают цвет рамки на
    # чёрном и передают управление правильной функции BIOS с теми же
    # AF/HL/SP. Заглушка BIOS оставляет отметку о фактическом вызове.
    for helper, bios, marker in ((0x3424, 0xC818, 0x18), (0x342E, 0xC809, 0x09)):
        outputs = []
        for name, game, entry in (("direct", fixed, bios), ("frame-color", dark, helper)):
            data = memory(game)
            data[0xFFF8] = 0x29
            stub = bytes.fromhex("f5 3e") + bytes([marker]) + bytes.fromhex("32 10 60 f1 c9")
            data[bios:bios + len(stub)] = stub
            code = (bytes.fromhex("31 ff 7f 21 d7 a5 e5 f1 21 ef be 01 04 03 cd")
                    + word(entry) + bytes.fromhex("22 02 60 f5 e1 22 00 60")
                    + bytes.fromhex("21 00 00 39 22 04 60 c3 00 50"))
            data[0x4000:0x4000 + len(code)] = code
            output, _ = run(args.ticks, f"{name}-text-{helper:04x}", data)
            require(output[0x6010] == marker, "Wrong BIOS text entry")
            outputs.append(output)
        require(outputs[0][0x6000:0x6006] == outputs[1][0x6000:0x6006], "Text AF/HL/SP changed")
        require(outputs[1][0xFFF8] == repair.USER_PALETTE[3] == 0x70, "Text/frame color differs")

    # Реальный вывод надписей BIOS с проектным шрифтом: проверить
    # абсолютные координаты обеих строк, включая цифры таймера и уровень.
    data = memory(dark)
    dos = (ROOT / "bin/DOS.sys").read_bytes()
    for entry in (0xC809, 0xC818):
        data[entry:entry + 3] = dos[entry - 0xC000:entry - 0xC000 + 3]
    font = (ROOT / "assets/system-rom/FONT.FNT").read_bytes()
    data[0xE000:0xE000 + len(font)] = font
    data[0x8FE7:0x8FE9] = word(0xE000 // 8)
    data[0x8FEA] = 0
    data[0x8FEB] = 0xFF
    data[0x8FFA:0x8FFC] = word(0)
    data[0x0B82:0x0B86] = bytes((1, 2, 3, 4))
    code = bytes.fromhex(
        "31 ff 7f cd 99 01 2a fc 8f 22 00 60 "
        "cd 1c 01 2a fc 8f 22 02 60 c3 00 50"
    )
    data[0x4000:0x4000 + len(code)] = code
    output, _ = run(args.ticks, "hud-real-bios", data)
    require(output[0x6000:0x6004] == bytes((11, 45, 11, 110)), "HUD coordinates differ")
    require(any(output[0x9000:0xC000]), "HUD text was not drawn")
    require(all(output[i] == 0 for i in range(0x9000, 0xC000) if not 4 <= i % 256 <= 11),
            "HUD pixels outside expected vertical range")
    require(output[0xFFF8] == 0x70, "HUD text/frame color differs")

    report_path = TMP / "checks.json"
    report["hud_cursor_y"] = 11
    report["user_palette_bytes"] = 16
    report["frame_color_text_helpers"] = 2
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"OK: original failure reproduced; 16 levels, 4 x 16 color indices, text, AF/HL/SP; {report_path}")


if __name__ == "__main__":
    main()
